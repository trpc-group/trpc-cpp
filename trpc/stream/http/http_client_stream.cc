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

}  // namespace trpc::stream
