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

#include "trpc/common/status.h"
#include "trpc/coroutine/fiber_blocking_noncontiguous_buffer.h"
#include "trpc/server/server_context.h"
#include "trpc/stream/http/common.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::http {

class Request;
class Response;

}  // namespace trpc::http

namespace trpc::stream {

/// @brief HTTP stream message reader.
class HttpReadStream {
 public:
  enum ReadState {
    kGoodBit = 0,
    kWriteEndBit = 1 << 0,
    kEofBit = 1 << 1,
    kErrorBit = 1 << 2,
  };

  HttpReadStream(http::Request* request, size_t capacity, bool blocking)
      : request_(request), read_buffer_(capacity), blocking_(blocking) {}

  /// @brief Whether in the streaming blocking environment.
  /// @private For internal use purpose only.
  bool IsBlocking() const { return blocking_; }

  /// @brief Reads up to max_bytes bytes of data into item in a blocking manner, waiting at most until the expiry time.
  /// @param item the buffer used to store the data.
  /// @param max_bytes the max bytes to read.
  /// @param expiry the deadline for waiting.
  /// @return Returns kSuccStatus on success, kStreamStatusServerReadTimeout on timeout, kStreamStatusServerNetworkError
  ///         on error.
  /// @note The item is valid only if the status returned is success. If the return status is success but the size of
  ///       the item is less than max_bytes, it indicates that the end of the stream has been reached and subsequent
  ///       calls will return `kStreamStatusReadEof`.
  template <typename Clock, typename Dur>
  Status Read(NoncontiguousBuffer& item, size_t max_bytes, const std::chrono::time_point<Clock, Dur>& expiry) {
    if (state_ & kErrorBit) {
      return kStreamStatusServerNetworkError;
    } else if (state_ & kEofBit) {
      return kStreamStatusReadEof;
    }

    if (!read_buffer_.Cut(item, max_bytes, expiry)) {
      return kStreamStatusServerReadTimeout;
    }

    if (state_ & kErrorBit) {  // encounter network error during the Cut operation
      return kStreamStatusServerNetworkError;
    }

    // mark EOF if the data is less than max_bytes
    // The first encounter with EOF during reading is considered a normal state, while subsequent attempts to read will
    // return kStreamStatusReadEof.
    if (item.ByteSize() < max_bytes) {
      state_ |= kEofBit;
    }

    return kSuccStatus;
  }

  /// @brief Reads up to max_bytes bytes of data into item in a blocking manner using the default deadline.
  Status Read(NoncontiguousBuffer& item, size_t max_bytes) { return Read(item, max_bytes, default_deadline_); }

  /// @brief Reads up to max_bytes bytes of data into item in a blocking manner, waiting for the duration of the expiry
  ///        time.
  template <typename Rep, typename Period>
  Status Read(NoncontiguousBuffer& item, size_t max_bytes, const std::chrono::duration<Rep, Period>& expiry) {
    return Read(item, max_bytes, trpc::ReadSteadyClock() + expiry);
  }

  /// @brief Reads all data into item in a blocking manner.
  /// @param item the buffer used to store the data that was read.
  /// @param expiry the deadline or duration for waiting.
  /// @return Returns kSuccStatus on success, kStreamStatusServerReadTimeout on timeout, kStreamStatusServerNetworkError
  ///         on error.
  /// @note The item is valid only if the status returned is success. If the call is successful, subsequent calls will
  ///       always return `kStreamStatusReadEof`.
  template <typename T>
  Status ReadAll(NoncontiguousBuffer& item, const T& expiry) {
    return Read(item, std::numeric_limits<size_t>::max(), expiry);
  }

  /// @brief Reads all data into item in a blocking manner using the default deadline.
  Status ReadAll(NoncontiguousBuffer& item) { return ReadAll(item, default_deadline_); }

  /// @brief Gets the total length of the message content that has been stored.
  /// @private For internal use purpose only.
  size_t Size() const;

  /// @brief Gets the maximum length of the message content allowed to be stored.
  /// @private For internal use purpose only.
  size_t Capacity() const { return read_buffer_.Capacity(); }

  /// @brief Sets the maximum length of the message content allowed to be stored.
  /// @private For internal use purpose only.
  void SetCapacity(size_t capacity) { read_buffer_.SetCapacity(capacity); }

  /// @brief Gets the default timeout deadline.
  const std::chrono::steady_clock::time_point& GetDefaultDeadline() const { return default_deadline_; }

  /// @brief Sets the default timeout deadline.
  void SetDefaultDeadline(std::chrono::steady_clock::time_point default_deadline) {
    default_deadline_ = default_deadline;
  }

  /// @brief Closes the stream reader.
  void Close(ReadState state = kEofBit);

  /// @brief Stores the received message content into the reader.
  /// @note This interface is for internal use only and should not be used by users.
  /// @private For internal use purpose only.
  bool Write(NoncontiguousBuffer&& item);

  /// @brief Marks the completion of receiving the message content.
  /// @note This interface is for internal use only and should not be used by users.
  /// @private For internal use purpose only.
  void WriteEof() { Close(kWriteEndBit); }

  /// @brief Moves the message data written through the Write interface to request_.
  /// @note This interface is for internal use only and should not be used by users.
  ///       It will only return success when the end of the stream has been reached.
  /// @private For internal use purpose only.
  Status AppendToRequest(size_t max_body_size);

 private:
  uint32_t state_{kGoodBit};
  http::Request* request_;
  FiberBlockingNoncontiguousBuffer read_buffer_;
  std::chrono::steady_clock::time_point default_deadline_;
  bool blocking_;
};

/// @brief HTTP stream message writer.
class HttpWriteStream {
 public:
  HttpWriteStream(http::Response* response, ServerContext* context) : response_(response), context_(context) {}

  /// @brief Sends the response header.
  /// @return Returns kSuccStatus on success, kStreamStatusServerNetworkError on error, kStreamStatusServerWriteTimeout
  ///         on timeout.
  Status WriteHeader();

  /// @brief Sends the response content.
  /// @param item the content to send
  /// @return Returns kSuccStatus on success, kStreamStatusServerNetworkError on error, kStreamStatusServerWriteTimeout
  ///         on timeout, kStreamStatusServerWriteContentLengthError when non-chunked response's write length not equal
  ///         to Content-Length
  Status Write(NoncontiguousBuffer&& item);

  /// @brief Finishes sending the message content.
  /// @return Returns kSuccStatus on success, kStreamStatusServerNetworkError on error, kStreamStatusServerWriteTimeout
  ///         on timeout, kStreamStatusServerWriteContentLengthError when non-chunked response's write length not equal
  ///         to Content-Length
  Status WriteDone();

  /// @brief Gets the maximum allowed length of message content that can be sent.
  /// @private For internal use purpose only.
  size_t Capacity() const;

  /// @brief Sets the maximum allowed length of message content that can be sent.
  /// @private For internal use purpose only.
  void SetCapacity(size_t capacity);

  /// @brief Closes the writer. Automatically write out the header/ending if they haven't been written yet.
  void Close() {
    if (!context_->IsResponse()) {
      WriteHeader();
      WriteDone();
    }
  }

 private:
  constexpr static ssize_t kChunked = -1;

  enum HttpWriterState {
    kInitial = 0,
    kHeaderWritten = 1 << 0,
    kWriteDone = 1 << 1,
  };

  uint32_t state_{kInitial};
  http::Response* response_;
  ServerContext* context_;
  ssize_t content_length_{0};
  size_t written_size_{0};
};

/// @private For internal use purpose only.
using TransientHttpReadStream = detail::TransientWrapper<HttpReadStream>;
/// @private For internal use purpose only.
using TransientHttpWriteStream = detail::TransientWrapper<HttpWriteStream>;

}  // namespace trpc::stream
