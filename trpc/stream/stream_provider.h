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

#include <any>
#include <memory>
#include <optional>
#include <utility>

#include "trpc/common/future/future.h"
#include "trpc/common/status.h"
#include "trpc/serialization/serialization.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::stream {

/// @brief A wrapper of exception for streaming error.
class StreamError : public ExceptionBase {
 public:
  explicit StreamError(Status&& status) : status_(std::move(status)) { ret_code_ = status_.GetFrameworkRetCode(); }
  explicit StreamError(const Status& status) : status_(status) { ret_code_ = status_.GetFrameworkRetCode(); }

  const char* what() const noexcept override {
    if (what_.empty()) what_ = status_.ToString();
    return what_.data();
  }

  const Status& GetStatus() const { return status_; }

 private:
  Status status_;
  mutable std::string what_;
};

/// @brief A wrapper of exception for timeout error.
class Timeout : public ExceptionBase {
 public:
  explicit Timeout(Status&& status) : status_(std::move(status)) { ret_code_ = status_.GetFrameworkRetCode(); }
  explicit Timeout(const Status& status) : status_(status) { ret_code_ = status_.GetFrameworkRetCode(); }

  const char* what() const noexcept override {
    if (what_.empty()) what_ = status_.ToString();
    return what_.data();
  }

  const Status& GetStatus() const { return status_; }

 private:
  Status status_;
  mutable std::string what_;
};

/// @brief Metadata of message content (reserved).
struct MessageContentMetadata {
  // Content type, it indicates how to decode or encode content of message, e.g. proto, json, proto by default.
  uint8_t content_type{0};
  // Encoding type, it indicates how to compress or decompress content of message, e.g. gzip,snappy, none by default.
  uint8_t content_encoding{0};
};

/// @brief Message content encoding or decoding operation options. Usually, it's used to (de)serialize content of
/// message.
struct MessageContentCodecOptions {
  serialization::Serialization* serialization{nullptr};
  uint32_t content_type;
};

struct StreamOptions;

/// @brief Status code of streaming RPC.
enum class StreamStatus : int8_t {
  // EOF, end of stream.
  kStreamEof = -99,
  // A good stream, it indicates stream is ready to read or write.
  kStreamGood = 0,
};

/// @brief An interface representing the ability to read a message from stream or write a message to stream.
/// Is provides both synchronous or asynchronous interfaces to accommodate different programming scenarios.
class StreamReaderWriterProvider : public RefCounted<StreamReaderWriterProvider> {
 public:
  virtual ~StreamReaderWriterProvider() = default;

  /// @brief Reads a message from the stream with optional timeout.
  ///
  /// @param msg is a pointer to the streaming message, which will be updated to the message read.
  /// @param timeout is the timeout for waiting, the function will block until a message is ready
  ///   - -1: wait indefinitely, >=0: wait for timeout in milliseconds.
  /// @return Returns status of the reading operation.
  virtual Status Read(NoncontiguousBuffer* msg, int timeout = -1) {
    TRPC_LOG_ERROR("Unimplemented");
    return kUnknownErrorStatus;
  }

  /// @brief Asynchronously reads a message from the stream with optional timeout.
  virtual Future<std::optional<NoncontiguousBuffer>> AsyncRead(int timeout = -1) {
    return MakeExceptionFuture<std::optional<NoncontiguousBuffer>>(CommonException("Unimplemented"));
  }

  /// @brief Writes a message to the stream.
  ///
  /// @param msg is a message to be sent.
  /// @return Return a status of writing operation.
  virtual Status Write(NoncontiguousBuffer&& msg) {
    TRPC_LOG_ERROR("Unimplemented");
    return kUnknownErrorStatus;
  }

  /// @brief Asynchronously writes a message to the stream.
  virtual Future<> AsyncWrite(NoncontiguousBuffer&& msg) {
    return MakeExceptionFuture<>(CommonException("Unimplemented"));
  }

  /// @brief It indicates all messages were written. It will send EOF message to the remote peer.
  virtual Status WriteDone() {
    TRPC_LOG_ERROR("Unimplemented");
    return kUnknownErrorStatus;
  }

  /// @brief Starts the steam loop.
  virtual Status Start() {
    TRPC_LOG_ERROR("Unimplemented");
    return kUnknownErrorStatus;
  }

  /// @brief Asynchronously starts the steam loop.
  virtual Future<> AsyncStart() { return MakeExceptionFuture<>(CommonException("Unimplemented")); }

  /// @brief Finishes the stream, and returns the final RPC execution result.
  virtual Status Finish() {
    TRPC_LOG_ERROR("Unimplemented");
    return kUnknownErrorStatus;
  }

  /// @brief Asynchronously finishes the stream.
  virtual Future<> AsyncFinish() { return MakeExceptionFuture<>(CommonException("Unimplemented")); }

  /// @brief Closes the stream if RPC call finished.
  virtual void Close(Status status) { TRPC_LOG_ERROR("Unimplemented"); }

  /// @brief Resets the stream if an error occurred.
  virtual void Reset(Status status) { TRPC_LOG_ERROR("Unimplemented"); }

  /// @brief Returns the inner status of stream.
  virtual Status GetStatus() const = 0;

  /// @brief Returns the mutable options of stream.
  virtual StreamOptions* GetMutableStreamOptions() { return nullptr; }

  /// @brief Returns the code of encoding error.
  virtual int GetEncodeErrorCode() { return -1; }

  /// @brief Returns the code of decoding error.
  virtual int GetDecodeErrorCode() { return -1; }

  /// @brief Returns the code of timeout error.
  virtual int GetReadTimeoutErrorCode() { return -1; }

  /// @brief Returns EOF status.
  Status GetEofStatus() const { return Status(static_cast<int>(StreamStatus::kStreamEof), 0, "stream EOF"); }
};

using StreamReaderWriterProviderPtr = RefPtr<StreamReaderWriterProvider>;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// @brief A wrapper of streaming error.
class ErrorStreamProvider : public StreamReaderWriterProvider {
 public:
  explicit ErrorStreamProvider(Status&& status) : status_(std::move(status)) {}

  ErrorStreamProvider() : status_(Status{-1, 0, "error"}) {}

  ~ErrorStreamProvider() override = default;

  Status Read(NoncontiguousBuffer* msg, int timeout = -1) override { return GetStatus(); }

  Future<std::optional<NoncontiguousBuffer>> AsyncRead(int timeout = -1) override {
    return MakeExceptionFuture<std::optional<NoncontiguousBuffer>>(StreamError(GetStatus()));
  }

  Status Write(NoncontiguousBuffer&& msg) override { return GetStatus(); }

  Status WriteDone() override { return GetStatus(); }

  Status Start() override { return GetStatus(); }

  Future<> AsyncStart() override {  // Exception or Ready?
    return MakeExceptionFuture<>(StreamError(GetStatus()));
  }

  Status Finish() override { return GetStatus(); }

  Future<> AsyncFinish() override { return MakeExceptionFuture<>(StreamError(GetStatus())); }

  void Close(Status status) override {}

  void Reset(Status status) override {}

  inline Status GetStatus() const override { return status_; }

 private:
  Status status_;
};
using ErrorStreamProviderPtr = RefPtr<ErrorStreamProvider>;

}  // namespace trpc::stream
