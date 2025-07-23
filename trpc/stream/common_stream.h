//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <optional>
#include <queue>

#include "trpc/codec/protocol.h"
#include "trpc/common/async_timer.h"
#include "trpc/stream/fiber_stream_job_scheduler.h"
#include "trpc/stream/stream_var.h"
#include "trpc/util/function.h"

namespace trpc::stream {

/// @brief Implementation of the common part of stream entities based on the concept of stream frames and fully
/// asynchronous stream interfaces.
/// Scope: Currently, tRPC streaming and gRPC streaming are such implementations.
class CommonStream : public StreamReaderWriterProvider {
 public:
  explicit CommonStream(StreamOptions&& options);

  ~CommonStream() override;

  /// @brief Reads a message from the stream with optional timeout.
  ///
  /// @param msg is a pointer to the streaming message, which will be updated to the message read.
  /// @param timeout is the timeout for waiting, the function will block until a message is ready
  ///   - -1: wait indefinitely, >=0: wait for timeout in milliseconds.
  /// @return Returns status of the reading operation.
  Status Read(NoncontiguousBuffer* msg, int timeout = -1) override;

  /// @brief Asynchronously reads a message from the stream with optional timeout.
  Future<std::optional<NoncontiguousBuffer>> AsyncRead(int timeout = -1) override;

  /// @brief Writes a message to the stream.
  ///
  /// @param msg is a message to be sent.
  /// @return Return a status of writing operation.
  Status Write(NoncontiguousBuffer&& msg) override;

  /// @brief Asynchronously writes a message to the stream.
  Future<> AsyncWrite(NoncontiguousBuffer&& msg) override;

  /// @brief It indicates all messages were written. It will send EOF message to the remote peer.
  Status WriteDone() override;

  /// @brief Starts the steam loop.
  Status Start() override;

  /// @brief Asynchronously starts the steam loop.
  Future<> AsyncStart() override;

  /// @brief Finishes the stream, and returns the final RPC execution result.
  Status Finish() override { return GetStatus(); }

  /// @brief Asynchronously finishes the stream.
  Future<> AsyncFinish() override;

  /// @brief Returns the inner status of stream.
  Status GetStatus() const override;

  /// @brief Closes the stream if RPC call finished.
  void Close(Status status) override {}

  /// @brief Resets the stream if an error occurred.
  void Reset(Status status) override {}

  /// @brief Pushes the received stream protocol frame message into the receive queue.
  virtual void PushRecvMessage(StreamRecvMessage&& msg);

  /// @brief Pushes the stream protocol frame message to be sent into the send queue.
  virtual RetCode PushSendMessage(StreamSendMessage&& msg, bool push_front = false);

  /// @brief Stops the stream loop.
  void Stop();

  /// @brief Waits for the stream loop to end.
  void Join();

  /// @brief Returns the ID of stream.
  uint32_t GetId() const { return options_.stream_id; }

  /// @brief Returns the mutable options of stream.
  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  /// @brief Reports whether stream is terminate.
  bool IsTerminate() const { return fiber_entity_ ? fiber_entity_->IsTerminate() : true; }

 protected:
  /// @brief Processes the stream protocol frame message received on the stream.
  /// @param msg is the stream protocol frame message to be processed.
  /// @return Returns 0 for successful processing, and other values represent stream message processing errors.
  RetCode HandleRecvMessage(StreamRecvMessage&& msg);

  /// @brief Processes the stream protocol frame message to be sent on the stream.
  /// @param msg The stream protocol frame message to be processed.
  /// @return Returns 0 for successful processing, and other values represent stream message processing errors.
  RetCode HandleSendMessage(StreamSendMessage&& msg);

 protected:
  /// @brief Stream state.
  enum class State : uint8_t {
    kClosed = 0,
    kIdle,
    kInit,
    kOpen,
    kLocalClosed,
    kRemoteClosed,
  };

  /// @brief Types of behaviors for sending/processing frame messages on the stream, used for stream state checking.
  enum Action {
    kSendInit = 0x01,
    kSendData = 0x02,
    kSendFeedback = 0x04,
    kSendClose = 0x08,
    kHandleInit = 0x10,
    kHandleData = 0x20,
    kHandleFeedback = 0x40,
    kHandleClose = 0x80,
  };

  /// @brief Stream reader/writer options.
  struct ReaderWriterOptions {
    /// Read message queue, std::optional is empty for EOF.
    std::queue<std::optional<NoncontiguousBuffer>> reader_msgs;
    /// Mutex for message reading, pushing, and stream status information updating.
    mutable FiberMutex mutex;
    mutable FiberConditionVariable cond;
    /// Asynchronous readiness.
    Promise<> ready;
    /// Asynchronous read.
    std::optional<Promise<std::optional<NoncontiguousBuffer>>> pending;
    uint64_t pending_timer_id{iotimer::InvalidID};
    /// Asynchronous completion.
    Promise<> done;
    enum SimpleState { NotReady = 0, Ready, Finished, Error } simple_state{NotReady};
    /// Record internal operational status of stream reader/writer.
    Status status;
  };

 protected:
  /// @brief Handles INIT frame message in the protocol.
  virtual RetCode HandleInit(StreamRecvMessage&& msg) = 0;

  /// @brief Handles DATA frame message in the protocol.
  virtual RetCode HandleData(StreamRecvMessage&& msg) = 0;

  /// @brief Handles FEEDBACK frame message in the protocol.
  virtual RetCode HandleFeedback(StreamRecvMessage&& msg) = 0;

  /// @brief Handles CLOSE frame message in the protocol.
  virtual RetCode HandleClose(StreamRecvMessage&& msg) = 0;

  /// @brief Handles RESET frame message in the protocol.
  virtual RetCode HandleReset(StreamRecvMessage&& msg) = 0;

  /// @brief Sends INIT frame message in the protocol.
  virtual RetCode SendInit(StreamSendMessage&& msg) = 0;

  /// @brief Sends DATA frame message in the protocol.
  virtual RetCode SendData(StreamSendMessage&& msg) = 0;

  /// @brief Sends FEEDBACK frame message in the protocol.
  virtual RetCode SendFeedback(StreamSendMessage&& msg) = 0;

  /// @brief Sends CLOSE frame message in the protocol.
  virtual RetCode SendClose(StreamSendMessage&& msg) = 0;

  /// @brief Sends RESET frame message in the protocol.
  virtual RetCode SendReset(StreamSendMessage&& msg) = 0;

 protected:
  void OnReady();

  void OnData(NoncontiguousBuffer&& msg);

  void OnFeedback(int window_increment) {}

  void OnFinish(Status status);

  virtual void OnError(Status status);

  void NotifyPending();

  void PendingDone();

 protected:
  static std::string_view StreamStateToString(State state);

  static std::string_view StreamActionToString(Action action);

  static std::string_view StreamMessageCategoryToString(StreamMessageCategory c);

  State GetState() const { return state_; }

  void SetState(State state) { state_ = state; }

  void SetStreamVar(StreamVarPtr&& stream_var) { stream_var_ = std::move(stream_var); }

  const StreamVarPtr& GetStreamVar() const { return stream_var_; }

  bool Send(NoncontiguousBuffer&& buffer);

  std::chrono::milliseconds GetTimeout(int timeout) {
    return std::chrono::milliseconds((timeout < 0) ? INT_MAX : timeout);
  }

  template <class T>
  T CriticalSection(Function<T()>&& cb) const {
    if (options_.fiber_mode) {
      std::unique_lock<FiberMutex> _(reader_writer_options_.mutex);
      return cb();
    } else {
      return cb();
    }
  }

 protected:
  ReaderWriterOptions reader_writer_options_;

 private:
  StreamOptions options_;
  State state_{State::kIdle};
  StreamVarPtr stream_var_{nullptr};
  RefPtr<FiberStreamJobScheduler> fiber_entity_{nullptr};
};
}  // namespace trpc::stream
