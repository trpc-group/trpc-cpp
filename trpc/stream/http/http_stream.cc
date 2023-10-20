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

#include "trpc/stream/http/http_stream.h"

#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/util.h"

namespace trpc::stream {

namespace {

Status ContextStatusToStreamStatus(Status&& status, const std::function<void()>& succ_callback) {
  if (TRPC_LIKELY(status.OK())) {
    succ_callback();
  } else if (status.GetFrameworkRetCode() == -1) {
    status = kStreamStatusServerNetworkError;
  } else if (status.GetFrameworkRetCode() == -2) {
    status = kStreamStatusServerWriteTimeout;
  }
  return std::move(status);
}

Connection* GetConnectionFromContext(ServerContext* context) {
  return static_cast<Connection*>(context->GetReserved());
}

}  // namespace

size_t HttpReadStream::Size() const {
  return blocking_ ? read_buffer_.ByteSize() : request_->GetNonContiguousBufferContent().ByteSize();
}

bool HttpReadStream::Write(NoncontiguousBuffer&& item) {
  if (state_ != kGoodBit) {
    return false;
  }
  if (blocking_) {
    read_buffer_.AppendAlways(std::move(item));
  } else {
    request_->GetMutableNonContiguousBufferContent()->Append(std::move(item));
  }
  return true;
}

void HttpReadStream::Close(ReadState state) {
  state_ |= state;
  if (blocking_) {
    read_buffer_.Stop();
  }
}

Status HttpReadStream::AppendToRequest(size_t max_body_size) {
  if (request_->ContentLength() > max_body_size) {
    return kStreamStatusServerMessageExceedLimit;
  }
  if (blocking_) {
    NoncontiguousBuffer buffer;
    Status status = Read(buffer, max_body_size + 1);
    if (status.OK()) {
      if (!(state_ & kEofBit)) {
        return kStreamStatusServerMessageExceedLimit;
      }
      request_->GetMutableNonContiguousBufferContent()->Append(std::move(buffer));
    }
    return status;
  }
  return kSuccStatus;
}

Status HttpWriteStream::WriteHeader() {
  if (!(state_ & kHeaderWritten)) {
    NoncontiguousBufferBuilder builder;
    if (response_->GetStatus() < http::Response::StatusCode::kOk) {  // 1xx informational response
      response_->ResponseFirstLine(builder);
      builder.Append(http::kEmptyLine);
      return ContextStatusToStreamStatus(context_->SendResponse(builder.DestructiveGet()),
                                         [&] { response_->SetStatus(http::HttpResponse::StatusCode::kOk); });
    } else {
      const std::string& content_length = response_->GetHeader(http::kHeaderContentLength);
      content_length_ = content_length.empty() ? kChunked : http::ParseContentLength(content_length).value_or(0);
      if (content_length_ == kChunked) {
        response_->SetHeader(http::kHeaderTransferEncoding, http::kTransferEncodingChunked);
      }
      response_->SerializeHeaderToString(builder);
      return ContextStatusToStreamStatus(context_->SendResponse(builder.DestructiveGet()), [&] {
        state_ |= kHeaderWritten;
        context_->SetResponse(false);
      });
    }
  }
  return kSuccStatus;
}

Status HttpWriteStream::Write(NoncontiguousBuffer&& item) {
  while (!(state_ & kHeaderWritten)) {  // ensure that the header has been sent completely
    if (Status headerStatus = WriteHeader(); !headerStatus.OK()) {
      return headerStatus;
    }
  }
  size_t item_size = item.ByteSize();
  if (content_length_ != kChunked && written_size_ + item_size > static_cast<size_t>(content_length_)) {
    return kStreamStatusServerWriteContentLengthError;
  }
  if (content_length_ == kChunked) {
    NoncontiguousBufferBuilder builder;
    builder.Append(HttpChunkHeader(item_size));
    builder.Append(std::move(item));
    builder.Append(http::kEndOfChunkMarker);
    item = builder.DestructiveGet();
  }
  return ContextStatusToStreamStatus(context_->SendResponse(std::move(item)), [&] { written_size_ += item_size; });
}

Status HttpWriteStream::WriteDone() {
  if (!(state_ & kWriteDone)) {
    if (content_length_ == kChunked) {  // chunked response - send end of chunked marker
      return ContextStatusToStreamStatus(context_->SendResponse(CreateBufferSlow(http::kEndOfChunkedResponseMarker)),
                                         [&] { state_ |= kWriteDone; });
    } else if (written_size_ != static_cast<size_t>(content_length_)) {  // non-chunked response - check content-length
      context_->CloseConnection();
      return kStreamStatusServerWriteContentLengthError;
    }
    state_ |= kWriteDone;
  }
  return kSuccStatus;
}

size_t HttpWriteStream::Capacity() const { return GetConnectionFromContext(context_)->GetSendQueueCapacity(); }

void HttpWriteStream::SetCapacity(size_t capacity) {
  GetConnectionFromContext(context_)->SetSendQueueCapacity(capacity);
}

}  // namespace trpc::stream
