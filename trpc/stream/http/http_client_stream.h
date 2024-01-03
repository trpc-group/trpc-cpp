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

#include "trpc/client/client_context.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/filter/client_filter_controller.h"
#include "trpc/stream/http/http_stream.h"
#include "trpc/stream/http/http_stream_provider.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/util/http/common.h"
#include "trpc/util/http/response.h"

namespace trpc::stream {

/// @brief Implementation of HTTP client stream.
/// @note Used internally by framework.
/// @private For internal use purpose only.
class HttpClientStream : public HttpStreamReaderWriterProvider {
 public:
  /// @brief Stream state.
  ///        State transition logic: create stream (kInitial)
  ///                                  -> SendRequestHeader success (kReading) -> receive eof (cancel kReading)
  ///                                  -> Write success (kWriting) -> WriteDone (cancel kWriting)
  ///                                  -> Close (kClosed)
  ///                                  -> Read reach end (kReadEof)
  /// @note If the final state has `kWriting` or `kReading`, it indicates that the read/write stream has not ended,
  ///       which is judged as an exception.
  ///       If both kReading and kClosed are present simultaneously, it indicates that the connection was disconnected
  ///       before receiving all the data, which is considered an exception.
  enum State {
    kInitial = 0,       ///< Initial state.
    kWriting = 1 << 0,  ///< Stream is writing by user.
    kReading = 1 << 1,  ///< Streaming is reading by user.
    kReadEof = 1 << 2,  ///< EOF.
    kClosed = 1 << 3,   ///< Stream is Closed.
  };

  explicit HttpClientStream(StreamOptions&& options) : options_(std::move(options)) {
    try {
      auto client_context = std::any_cast<const ClientContextPtr>(options_.context.context);
    } catch (const std::bad_any_cast& e) {
      state_ |= kClosed;
      status_ = kStreamStatusClientNetworkError;
      status_.SetErrorMessage("create stream failed, no client context");
    }
  }

  HttpClientStream(const Status& status, bool closing) {
    status_ = status;
    if (closing) {
      state_ |= kReading | kClosed;
    }
  }

  ~HttpClientStream() override = default;

  /// @brief Reads response headers.
  /// @note By default, it blocks until the timeout specified in `client_context` is reached.
  Status ReadHeaders(int& code, trpc::http::HttpHeader& http_header) {
    return ReadHeaders(code, http_header, default_deadline_);
  }

  /// @brief Reads response headers (with a custom timeout).
  /// @note Blocks until the time specified by `expiry` has passed.
  template <typename Rep, typename Period>
  Status ReadHeaders(int& code, trpc::http::HttpHeader& http_header, const std::chrono::duration<Rep, Period>& expiry) {
    return ReadHeaders(code, http_header, trpc::ReadSteadyClock() + expiry);
  }

  /// @brief Reads response header (with custom timeout)
  /// @note Block until the expiry time is reached and timeout occurs.
  template <typename Clock, typename Dur>
  Status ReadHeaders(int& code, trpc::http::HttpHeader& http_header,
                     const std::chrono::time_point<Clock, Dur>& expiry) {
    std::unique_lock<FiberMutex> lk(http_response_mutex_);
    if (!http_response_can_read_.wait_until(lk, expiry, [this] { return http_response_ || (state_ & kClosed); })) {
      return kStreamStatusClientReadTimeout;
    }

    if (http_response_) {
      code = http_response_->GetStatus();
      http_header = http_response_->GetHeader();
      return kDefaultStatus;
    }
    return kStreamStatusClientNetworkError;
  }

  /// @brief Reads response message content.
  /// @note By default, it blocks until the timeout specified in `client_context` is reached.
  Status Read(NoncontiguousBuffer& item, size_t max_bytes) { return Read(item, max_bytes, default_deadline_); }

  /// @brief Reads response message content (with a custom timeout).
  /// @note Blocks until the time specified by `expiry` has passed.
  template <typename Rep, typename Period>
  Status Read(NoncontiguousBuffer& item, size_t max_bytes, const std::chrono::duration<Rep, Period>& expiry) {
    return Read(item, max_bytes, trpc::ReadSteadyClock() + expiry);
  }

  /// @brief Reads response message content (with a custom timeout).
  /// @note Blocks until the specified `expiry` time point is reached.
  template <typename Clock, typename Dur>
  Status Read(NoncontiguousBuffer& item, size_t max_bytes, const std::chrono::time_point<Clock, Dur>& expiry) {
    if ((state_ & (kClosed | kReading)) == (kClosed | kReading)) {  // closed while reading
      return kStreamStatusClientNetworkError;
    }

    if (state_ & kReadEof) {
      return kStreamStatusReadEof;
    }

    if (!read_buffer_.Cut(item, max_bytes, expiry)) {
      return kStreamStatusClientReadTimeout;
    }

    // The end of the `Cut` operation may be due to a normal return, or it may be due to an interruption from
    // `Close/EOF`.
    // If the framework network has already received EOF, it should still return normally even if the stream
    // (connection) has been closed.
    // So an error should only occur when the stream (connection) is closed and the framework network has not yet
    // received EOF.
    if ((state_ & (kClosed | kReading)) == (kClosed | kReading)) {
      return kStreamStatusClientNetworkError;
    }

    if (item.ByteSize() < max_bytes) {
      // The first occurrence of `EOF` is considered to be normal, and only subsequent retries to read will return
      // `kStreamStatusReadEof`.
      state_ |= kReadEof;
    }

    return kDefaultStatus;
  }

  /// @brief Read all response message content.
  /// @note By default, it blocks until the timeout specified in `client_context` is reached.
  Status ReadAll(NoncontiguousBuffer& item) { return ReadAll(item, default_deadline_); }

  /// @brief Read all response message content (with a custom timeout).
  /// @note Blocks until the time calculated by `expiry` has passed.
  template <typename T>
  Status ReadAll(NoncontiguousBuffer& item, const T& expiry) {
    return Read(item, std::numeric_limits<size_t>::max(), expiry);
  }

  /// @brief Sends a request message.
  Status Write(NoncontiguousBuffer&& item);

  /// @brief Sends a EOF message.
  Status WriteDone();

  /// @brief Closes the stream.
  void Close();

  /// @brief Recycles the connection (can only be called by the destructor of the connection owner ReaderWriter).
  void RecycleConnector();

  /// @brief Gets the total length of readable message content.
  size_t Size() const { return read_buffer_.ByteSize(); }

  /// @brief Gets the maximum length of message content that is allowed to be read.
  size_t Capacity() const { return read_buffer_.Capacity(); }

  /// @brief Sets the maximum length of message content that is allowed to be read.
  void SetCapacity(size_t capacity) { read_buffer_.SetCapacity(capacity); }

  /// @brief Sets HTTP request protocol.
  void SetHttpRequestProtocol(trpc::HttpRequestProtocol* req_protocol) { req_protocol_ = req_protocol; }

  /// @brief Sets the method of request.
  void SetMethod(trpc::http::OperationType http_method) { http_method_ = http_method; }

  /// @brief Sends the header message of request.
  Status SendRequestHeader();

  /// @brief Stores data in the receive queue.
  void PushDataToRecvQueue(NoncontiguousBuffer&& item) { read_buffer_.AppendAlways(std::move(item)); }

  /// @brief Stores the `EOF` flag in the receive queue, indicating that data reception is complete.
  void PushEofToRecvQueue() {
    state_ &= ~kReading;
    read_buffer_.Stop();
  }

  /// @brief Receives and process response (containing only HTTP response header information).
  void PushRecvMessage(std::any&& message);

  /// @brief Gets mutable stream options.
  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  /// @brief Sets the filter.
  void SetFilterController(ClientFilterController* filter_controller) { filter_controller_ = filter_controller; }

 private:
  using HttpStreamReaderWriterProvider::Close;
  using HttpStreamReaderWriterProvider::Read;

  Status SendMessage(const std::any& item, NoncontiguousBuffer&& send_data);

  FilterStatus RunMessageFilter(const FilterPoint& point, const ClientContextPtr& context);

 private:
  constexpr static ssize_t kChunked{-1};

  StreamOptions options_;
  uint32_t state_{kInitial};
  trpc::http::OperationType http_method_;
  ssize_t content_length_{0};
  size_t written_size_{0};
  trpc::HttpRequestProtocol* req_protocol_{nullptr};
  std::chrono::steady_clock::time_point default_deadline_;
  FiberMutex http_response_mutex_;
  FiberConditionVariable http_response_can_read_;
  std::optional<trpc::http::HttpResponse> http_response_;
  ClientFilterController* filter_controller_{nullptr};
};

using HttpClientStreamPtr = RefPtr<HttpClientStream>;

/// @brief Implementation of HTTP client stream reader/writer for user programing interface.
/// @note Used by the users.
class HttpClientStreamReaderWriter {
 public:
  explicit HttpClientStreamReaderWriter(HttpClientStreamPtr provider) : provider_(std::move(provider)) {}

  ~HttpClientStreamReaderWriter();

  HttpClientStreamReaderWriter(HttpClientStreamReaderWriter&& rw);
  HttpClientStreamReaderWriter& operator=(HttpClientStreamReaderWriter&& rw);

  HttpClientStreamReaderWriter(const HttpClientStreamReaderWriter&) = delete;
  HttpClientStreamReaderWriter& operator=(const HttpClientStreamReaderWriter&) = delete;

  /// @brief Gets status of stream.
  Status GetStatus() { return provider_->GetStatus(); }

  /// @brief Reads response headers.
  /// @note By default, it blocks until the timeout specified in `client_context` is reached.
  Status ReadHeaders(int& code, trpc::http::HttpHeader& http_header) {
    return provider_->ReadHeaders(code, http_header);
  }

  /// @brief Reads response message content (with a custom timeout).
  /// @note Blocks until the specified `expiry` time point is reached.
  /// @param[in] expiry may be std::chrono::milliseconds(3)
  ///       or trpc::ReadSteadyClock() + std::chrono::milliseconds(3)
  template <typename T>
  Status ReadHeaders(int& code, trpc::http::HttpHeader& http_header, const T& expiry) {
    return provider_->ReadHeaders(code, http_header, expiry);
  }

  /// @brief Reads response message content.
  /// @note By default, it blocks until the timeout specified in `client_context` is reached.
  Status Read(NoncontiguousBuffer& item, size_t max_bytes) { return provider_->Read(item, max_bytes); }

  /// @brief Reads response message content (with a custom timeout).
  /// @note Blocks until the time specified by `expiry` has passed.
  /// @param[in] expiry may be std::chrono::milliseconds(3)
  ///       or trpc::ReadSteadyClock() + std::chrono::milliseconds(3)
  template <typename T>
  Status Read(NoncontiguousBuffer& item, size_t max_bytes, const T& expiry) {
    return provider_->Read(item, max_bytes, expiry);
  }

  /// @brief Read all response message content.
  /// @note By default, it blocks until the timeout specified in `client_context` is reached.
  Status ReadAll(NoncontiguousBuffer& item) { return provider_->ReadAll(item); }

  /// @brief Read all response message content (with a custom timeout).
  /// @note Blocks until the time calculated by `expiry` has passed.
  /// @param[in] expiry may be std::chrono::milliseconds(3)
  ///       or trpc::ReadSteadyClock() + std::chrono::milliseconds(3)
  template <typename T>
  Status ReadAll(NoncontiguousBuffer& item, const T& expiry) {
    return provider_->ReadAll(item, expiry);
  }

  /// @brief Sends a request message.
  Status Write(NoncontiguousBuffer&& item) { return provider_->Write(std::move(item)); }

  /// @brief Sends a message.
  Status WriteDone() { return provider_->WriteDone(); }

  /// @brief Closes the stream.
  void Close() { provider_->Close(); }

 private:
  HttpClientStreamPtr provider_;
};

/// @brief Creates a HTTP client stream reader-writer.
/// @private For internal use purpose only.
inline HttpClientStreamReaderWriter Create(const HttpClientStreamPtr& provider) {
  TRPC_ASSERT(provider != nullptr && "stream reader writer provider is nullptr");
  return HttpClientStreamReaderWriter(provider);
}

}  // namespace trpc::stream
