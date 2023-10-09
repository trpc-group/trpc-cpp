//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <deque>
#include <memory>

#include "trpc/codec/protocol.h"
#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/stream/stream_message.h"
#include "trpc/stream/stream_provider.h"

namespace trpc::stream {

/// @brief Stream task scheduler, including receive queue and send queue.
/// When the stream coroutine is running, it will first process the receive queue and then process the send queue.
class FiberStreamJobScheduler : public RefCounted<FiberStreamJobScheduler> {
 public:
  using RetCode = stream::RetCode;

  explicit FiberStreamJobScheduler(uint32_t stream_id, StreamReaderWriterProvider* stream,
                                   std::function<RetCode(StreamRecvMessage&&)> handle_recv,
                                   std::function<RetCode(StreamSendMessage&&)> handle_send)
      : stream_id_(stream_id), stream_(stream), handle_recv_(handle_recv), handle_send_(handle_send) {}

  ~FiberStreamJobScheduler() = default;

  /// @brief Pushes the received stream protocol frame message into the receive queue.
  void PushRecvMessage(StreamRecvMessage&& msg);

  /// @brief Pushes the stream protocol frame message to be sent into the send queue. Some special flow frames
  /// (such as stream control frames) have high priority and are pushed to the head of the queue for sending.
  RetCode PushSendMessage(StreamSendMessage&& msg, bool push_front = false);

  /// @brief Stops the stream coroutine.
  void Stop();

  /// @brief Waits for the stream coroutine to end.
  void Join();

  /// @brief Returns the size of receiving queue.
  std::size_t GetRecvQueueSize() const { return stream_recv_msgs_.size(); }

  /// @brief Returns the size of sending queue.
  std::size_t GetSendQueueSize() const { return stream_send_msgs_.size(); }

  /// @brief Reports whether stream is terminate.
  bool IsTerminate() const;

 private:
  // @brief Coroutine function for processing stream protocol frame messages.
  // @note The coroutine is stopped by the stream messages pushed by PushRecvMessage/PushSendMessage.
  void Run();

 private:
  uint32_t stream_id_;
  StreamReaderWriterProvider* stream_;
  std::function<RetCode(StreamRecvMessage&&)> handle_recv_;
  std::function<RetCode(StreamSendMessage&&)> handle_send_;

  bool terminate_{false};
  bool done_{false};
  bool running_{false};
  std::deque<StreamRecvMessage> stream_recv_msgs_;
  std::deque<StreamSendMessage> stream_send_msgs_;
  mutable FiberMutex stream_mutex_;
  mutable FiberConditionVariable stream_cond_;
};

}  // namespace trpc::stream
