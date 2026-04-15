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

#include "trpc/stream/http/http_client_stream.h"

#include "trpc/stream/http/http_stream.h"
#include "trpc/util/http/util.h"

namespace trpc::stream {

inline Status ConvertConnSendError(int code) {
  // Convert the `send` method result of the underlying fiber connection into a stream return code.
  // `-2` represents a timeout, and other non-zero values represent a connection exception.
  if (code == -2) {
    return kStreamStatusClientWriteTimeout;
  }

  return kStreamStatusClientNetworkError;
}

Status HttpClientStream::Write(NoncontiguousBuffer&& item) {
  size_t item_size = item.ByteSize();
  NoncontiguousBufferBuilder builder;
  if (content_length_ == kChunked) {
    builder.Append(HttpChunkHeader(item.ByteSize()));
    builder.Append(std::move(item));
    builder.Append(http::kEndOfChunkMarker);
  } else if (written_size_ + item_size <= static_cast<size_t>(content_length_)) {
    builder.Append(std::move(item));
  } else {
    return kStreamStatusClientWriteContentLengthError;
  }

  state_ |= kWriting;

  Status status = SendMessage(options_.context.context, builder.DestructiveGet());
  if (status.OK()) {
    written_size_ += item_size;
  }
  return status;
}

Status HttpClientStream::WriteDone() {
  // The length of the sent message does not match the set length.
  if (content_length_ != kChunked && written_size_ != static_cast<size_t>(content_length_)) {
    return kStreamStatusClientWriteContentLengthError;
  }

  if (!(state_ & kWriting)) {
    return kDefaultStatus;
  }

  if (state_ & kClosed) {
    return kStreamStatusClientNetworkError;
  }

  if (content_length_ == kChunked) {
    NoncontiguousBufferBuilder builder;
    builder.Append(http::kEndOfChunkedResponseMarker);
    auto status = SendMessage(options_.context.context, builder.DestructiveGet());
    if (!status.OK()) {
      return status;
    }
  }

  state_ &= ~kWriting;

  return kDefaultStatus;
}

void HttpClientStream::Close() {
  WriteDone();
  state_ |= kClosed;
  // Simply sets the `stop_token_` flag in the queue to `true`.
  read_buffer_.Stop();
  http_response_can_read_.notify_one();
}

void HttpClientStream::RecycleConnector() {
  if (options_.stream_handler == nullptr) {
    return;
  }

  Close();
  options_.stream_handler->RemoveStream(0);

  int reason = 0;
  if (state_ & (kReading | kWriting)) {  // Stream is uncompleted.
    reason = kStreamStatusClientNetworkError.GetFrameworkRetCode();
  } else if (!http_response_ || !http_response_->IsConnectionReusable()) {  // If the header is not received or the
                                                                            // connection is not allowed to be reused.
    reason = TRPC_STREAM_CLIENT_READ_CLOSE_ERR;
  }

  options_.callbacks.on_close_cb(reason);

  auto client_context = std::any_cast<const ClientContextPtr>(options_.context.context);
  RunMessageFilter(FilterPoint::CLIENT_POST_RPC_INVOKE, client_context);
}

Status HttpClientStream::SendRequestHeader() {
  TRPC_ASSERT(req_protocol_);

  auto client_context = std::any_cast<const ClientContextPtr>(options_.context.context);
  RunMessageFilter(FilterPoint::CLIENT_PRE_RPC_INVOKE, client_context);

  // Accepts chunked data.
  req_protocol_->request->SetHeaderIfNotPresent(http::kHeaderAcceptTransferEncoding, http::kTransferEncodingChunked);
  if (req_protocol_->request->HasHeader(http::kHeaderTransferEncoding)) {
    content_length_ = kChunked;
    state_ |= kWriting;
  }

  // The business has set length information, and it is expected that the business will send data.
  const std::string& content_length = req_protocol_->request->GetHeader(http::kHeaderContentLength);
  if (!content_length.empty()) {
    if (content_length_ == kChunked) {
      state_ &= ~kWriting;
      status_ = kStreamStatusClientWriteContentLengthError;
      status_.SetErrorMessage("contentLength and chunked cannot be set at the same time");
      return status_;
    }

    content_length_ = http::ParseContentLength(content_length).value_or(0);
    state_ |= kWriting;
  }

  NoncontiguousBufferBuilder builder;
  req_protocol_->request->SerializeToString(builder);

  // Set the default timeout for waiting to receive the entire packet.
  default_deadline_ = trpc::ReadSteadyClock() + std::chrono::milliseconds(client_context->GetTimeout());
  state_ |= State::kReading;
  if (!SendMessage(options_.context.context, builder.DestructiveGet()).OK()) {
    status_ = kStreamStatusClientNetworkError;
    status_.SetErrorMessage("send request header failed");
    return status_;
  }

  return kDefaultStatus;
}

void HttpClientStream::PushRecvMessage(std::any&& message) {
  // There is only HTTP response header, no content.
  std::unique_lock<FiberMutex> lk(http_response_mutex_);
  http_response_ = std::any_cast<trpc::http::HttpResponse&&>(std::move(message));
  http_response_can_read_.notify_one();
}

Status HttpClientStream::SendMessage(const std::any& item, NoncontiguousBuffer&& send_data) {
  if (state_ & kClosed) {
    return kStreamStatusClientNetworkError;
  }

  if (int ret = options_.stream_handler->SendMessage(item, std::move(send_data)); ret != 0) {
    return ConvertConnSendError(ret);
  }

  return kDefaultStatus;
}

/// @brief Reads response header in a fiber-friendly way (non-blocking)
/// @note This method avoids blocking the entire fiber by using non-blocking reads
///       and yielding control to other fibers when data is not immediately available.
template <typename Clock, typename Dur>
Status HttpClientStream::ReadHeadersNonBlocking(int& code, trpc::http::HttpHeader& http_header,
                              const std::chrono::time_point<Clock, Dur>& expiry) {
  TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Starting header read operation");
  
  std::unique_lock<FiberMutex> lk(http_response_mutex_);
  
  // Check if headers are already available through normal mechanism
  if (http_response_) {
    TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Headers already available through normal mechanism");
    code = http_response_->GetStatus();
    http_header = http_response_->GetHeader();
    return kDefaultStatus;
  }
  
  // Check if stream is closed
  if (state_ & kClosed) {
    TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Stream is closed");
    return kStreamStatusClientNetworkError;
  }
  
  // Since we're in a separate thread (not fiber context), we need to actively poll
  // for the headers rather than waiting for a notification that may never come
  TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Will actively poll for headers");
  
  // Use the same clock type for comparison
  auto start_time = Clock::now();
  
  while (start_time < expiry) {
    // Unlock while we do non-critical work
    lk.unlock();
    
    // Small delay to prevent busy waiting
    if (::trpc::IsRunningInFiberWorker()) {
      TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Yielding fiber");
      FiberYield();
      FiberSleepFor(std::chrono::milliseconds(10));
    } else {
      TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Sleeping thread");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Lock again to check conditions
    lk.lock();
    
    // Check if headers became available
    if (http_response_) {
      TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Headers became available during polling");
      code = http_response_->GetStatus();
      http_header = http_response_->GetHeader();
      TRPC_FMT_DEBUG("ReadHeadersNonBlocking: HTTP Status: {}, Headers count: {}", 
                     code, http_header.Pairs().size());
      for (const auto& [key, value] : http_header.Pairs()) {
        TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Header: {}: {}", key, value);
      }
      return kDefaultStatus;
    }
    
    // Check if stream is closed
    if (state_ & kClosed) {
      TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Stream closed during polling");
      return kStreamStatusClientNetworkError;
    }
    
    TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Headers still not available, continuing to poll");
    start_time = Clock::now();
  }
  
  TRPC_FMT_DEBUG("ReadHeadersNonBlocking: Timeout reached while polling for headers");
  return kStreamStatusClientReadTimeout;
}

/// @brief Reads response header in a fiber-friendly way (non-blocking) with duration timeout
template <typename Rep, typename Period>
Status HttpClientStream::ReadHeadersNonBlocking(int& code, trpc::http::HttpHeader& http_header, 
                              const std::chrono::duration<Rep, Period>& expiry) {
  return ReadHeadersNonBlocking(code, http_header, trpc::ReadSteadyClock() + expiry);
}

FilterStatus HttpClientStream::RunMessageFilter(const FilterPoint& point, const ClientContextPtr& context) {
  if (filter_controller_) {
    return filter_controller_->RunMessageClientFilters(point, context);
  }
  return FilterStatus::REJECT;
}

HttpClientStreamReaderWriter::~HttpClientStreamReaderWriter() {
  if (provider_ == nullptr) return;
  // Cleanup the connection used by the stream.
  provider_->RecycleConnector();
  provider_ = nullptr;
}

HttpClientStreamReaderWriter::HttpClientStreamReaderWriter(HttpClientStreamReaderWriter&& rw) {
  this->provider_ = std::move(rw.provider_);
  rw.provider_ = nullptr;
}


HttpClientStreamReaderWriter& HttpClientStreamReaderWriter::operator=(HttpClientStreamReaderWriter&& rw) {
  if (this != &rw) {
    this->provider_ = std::move(rw.provider_);
    rw.provider_ = nullptr;
  }
  return *this;
}

// SSE-specific method implementations
Status HttpClientStream::ConfigureSseMode() {
  if (sse_mode_) {
    return kSuccStatus;  // Already configured
  }

  if (!req_protocol_) {
    return kStreamStatusClientNetworkError;
  }

  // Set SSE-specific headers
  req_protocol_->request->SetHeader("Accept", "text/event-stream");
  req_protocol_->request->SetHeader("Cache-Control", "no-cache");
  req_protocol_->request->SetHeader("Connection", "keep-alive");

  sse_mode_ = true;
  return kSuccStatus;
}

Status HttpClientStream::ReadSseEvent(http::sse::SseEvent& event, size_t max_bytes) {
  return ReadSseEvent(event, max_bytes, default_deadline_);
}

template <typename Clock, typename Dur>
Status HttpClientStream::ReadSseEvent(http::sse::SseEvent& event, size_t max_bytes,
                                      const std::chrono::time_point<Clock, Dur>& expiry) {
  if (!sse_mode_) {
    return kStreamStatusClientNetworkError;
  }

  // Read data from the stream
  NoncontiguousBuffer buffer;
  Status status = Read(buffer, max_bytes, expiry);
  if (!status.OK()) {
    return status;
  }

  // Convert buffer to string and append to SSE buffer
  std::string new_data = FlattenSlow(buffer);
  sse_buffer_ += new_data;

  // Try to parse complete SSE events
  std::vector<http::sse::SseEvent> events;
  if (!ParseSseEvents(buffer, events)) {
    return kStreamStatusClientNetworkError;
  }

  if (events.empty()) {
    // No complete event found, continue reading
    return ReadSseEvent(event, max_bytes, expiry);
  }

  // Return the first complete event
  event = events[0];
  
  // Remove the parsed event data from the buffer
  // This is a simplified approach - in a production implementation,
  // you'd want to track exactly how much data was consumed
  size_t event_end = sse_buffer_.find("\n\n");
  if (event_end != std::string::npos) {
    sse_buffer_ = sse_buffer_.substr(event_end + 2);
  }

  return kSuccStatus;
}

// Explicit template instantiations for common clock types
template Status HttpClientStream::ReadSseEvent<std::chrono::steady_clock, std::chrono::nanoseconds>(
    http::sse::SseEvent&, size_t, const std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>&);
template Status HttpClientStream::ReadSseEvent<std::chrono::system_clock, std::chrono::nanoseconds>(
    http::sse::SseEvent&, size_t, const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>&);

bool HttpClientStream::ParseSseEvents(const NoncontiguousBuffer& buffer, std::vector<http::sse::SseEvent>& events) {
  try {
    // Convert buffer to string
    std::string data = FlattenSlow(buffer);
    
    // Use the existing SSE parser
    events = trpc::http::sse::SseParser::ParseEvents(data);
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

// Explicit template instantiations for ReadHeadersNonBlocking
template Status trpc::stream::HttpClientStream::ReadHeadersNonBlocking<std::chrono::_V2::steady_clock, std::chrono::nanoseconds>(
    int&, trpc::http::HttpHeader&, const std::chrono::time_point<std::chrono::_V2::steady_clock, std::chrono::nanoseconds>&);
template Status trpc::stream::HttpClientStream::ReadHeadersNonBlocking<std::chrono::_V2::system_clock, std::chrono::nanoseconds>(
    int&, trpc::http::HttpHeader&, const std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::nanoseconds>&);

// Explicit template instantiations for ReadHeadersNonBlocking with duration
template Status trpc::stream::HttpClientStream::ReadHeadersNonBlocking<int64_t, std::milli>(
    int&, trpc::http::HttpHeader&, const std::chrono::duration<int64_t, std::milli>&);
template Status trpc::stream::HttpClientStream::ReadHeadersNonBlocking<int64_t, std::ratio<1, 1>>(
    int&, trpc::http::HttpHeader&, const std::chrono::duration<int64_t, std::ratio<1, 1>>&);

// Explicit template instantiations for ReadHeaders
template Status trpc::stream::HttpClientStream::ReadHeaders<std::chrono::_V2::steady_clock, std::chrono::nanoseconds>(
    int&, trpc::http::HttpHeader&, const std::chrono::time_point<std::chrono::_V2::steady_clock, std::chrono::nanoseconds>&);
template Status trpc::stream::HttpClientStream::ReadHeaders<std::chrono::_V2::system_clock, std::chrono::nanoseconds>(
    int&, trpc::http::HttpHeader&, const std::chrono::time_point<std::chrono::_V2::system_clock, std::chrono::nanoseconds>&);

// Explicit template instantiations for ReadHeaders with duration
template Status trpc::stream::HttpClientStream::ReadHeaders<int64_t, std::milli>(
    int&, trpc::http::HttpHeader&, const std::chrono::duration<int64_t, std::milli>&);
template Status trpc::stream::HttpClientStream::ReadHeaders<int64_t, std::ratio<1, 1>>(
    int&, trpc::http::HttpHeader&, const std::chrono::duration<int64_t, std::ratio<1, 1>>&);

}  // namespace trpc::stream
