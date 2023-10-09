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

#include "trpc/stream/http/async/server/stream.h"

#include <optional>

#include "trpc/stream/stream_message.h"

namespace trpc::stream {

const HttpRequestLine& HttpServerAsyncStream::GetRequestLine() { return start_line_; }

const http::PathParameters& HttpServerAsyncStream::GetParameters() const { return path_parameters_; }
http::PathParameters* HttpServerAsyncStream::GetMutableParameters() { return &path_parameters_; }

void HttpServerAsyncStream::SetParameters(http::Parameters&& param) {
  path_parameters_ = std::move(param);
}

http::HttpRequestPtr HttpServerAsyncStream::GetFullRequest() {
  TRPC_ASSERT(has_full_message_ && "http message must be full");
  auto req = std::make_shared<http::Request>();
  req->SetMethod(GetRequestLine().method);
  req->SetVersion(GetRequestLine().version);
  req->SetUrl(GetRequestLine().request_uri);
  req->SetParameters(GetParameters());
  *(req->GetMutableHeader()) = std::move(header_.value());
  std::optional<size_t> content_length = 0;
  if (read_mode_ == DataMode::kContentLength) {
    *content_length += check_data_.ByteSize();
    req->SetNonContiguousBufferContent(std::move(check_data_));
  } else {
    NoncontiguousBufferBuilder builder;
    while (!data_block_.empty()) {
      *content_length += data_block_.front().ByteSize();
      builder.Append(std::move(data_block_.front()));
      data_block_.pop_front();
    }
    req->SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  req->SetContentLength(content_length);
  return req;
}

void HttpServerAsyncStream::Reset(Status status) {
  OnError(status);
  Stop();
  auto& context = std::any_cast<ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  context->CloseConnection();
}

void HttpServerAsyncStream::OnError(Status status) {
  if (!GetStatus().OK()) {
    return;
  }
  HttpAsyncStream::OnError(status);
}

RetCode HttpServerAsyncStream::HandleRecvMessage(HttpStreamFramePtr&& msg) {
  switch (msg->GetFrameType()) {
    case HttpStreamFrame::HttpStreamFrameType::kRequestLine: {  // handle kRequestLine frame
      auto frame = static_cast<HttpStreamRequestLine*>(msg.Get());
      return HandleRequestLine(frame->GetMutableHttpRequestLine());
    }
    case HttpStreamFrame::HttpStreamFrameType::kFullMessage: {
      RetCode ret = HttpAsyncStream::HandleRecvMessage(std::move(msg));
      if (ret == RetCode::kError) {
        return ret;
      }
      auto context = std::any_cast<ServerContextPtr>(GetMutableStreamOptions()->context.context);
      STransportReqMsg req;
      req.context = context;
      GetMutableStreamOptions()->callbacks.handle_streaming_rpc_cb(&req);
      return RetCode::kSuccess;
    }
    default: {
      return HttpAsyncStream::HandleRecvMessage(std::move(msg));
    }
  }
}

RetCode HttpServerAsyncStream::HandleRequestLine(HttpRequestLine* start_line) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle start_line begin, {} {} {}", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id, start_line->method, start_line->request_uri,
                 start_line->version);

  if (read_state_ != State::kIdle) {
    std::string error_msg{"bad stream state"};
    error_msg.append(", action: ").append(StreamActionToString(Action::kHandleStartLine));
    error_msg.append(", stream state: ").append(StreamStateToString(read_state_));
    error_msg.append(", expected state: ").append(StreamStateToString(State::kIdle));
    TRPC_FMT_ERROR(error_msg);

    Status status{TRPC_STREAM_UNKNOWN_ERR, 0, std::move(error_msg)};
    OnError(status);
    return kError;
  }

  start_line_ = std::move(*start_line);

  read_state_ = State::kInit;

  if (!has_full_message_) {
    auto context = std::any_cast<ServerContextPtr>(GetMutableStreamOptions()->context.context);
    STransportReqMsg req;
    req.context = context;
    GetMutableStreamOptions()->callbacks.handle_streaming_rpc_cb(&req);
  }

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle start_line end", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id);

  return RetCode::kSuccess;
}

RetCode HttpServerAsyncStream::HandleSendMessage(HttpStreamFramePtr&& msg, NoncontiguousBuffer* out) {
  switch (msg->GetFrameType()) {
    case HttpStreamFrame::HttpStreamFrameType::kStatusLine: {
      auto frame = static_cast<HttpStreamStatusLine*>(msg.Get());
      return PreSendStatusLine(frame->GetMutableHttpStatusLine(), out);
    }
    default: {
      return HttpAsyncStream::HandleSendMessage(std::move(msg), out);
    }
  }
}

RetCode HttpServerAsyncStream::PreSendStatusLine(HttpStatusLine* status_line, NoncontiguousBuffer* out) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send start_line begin, {} {} {}", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id, status_line->version, status_line->status_code,
                 status_line->status_text);

  if (write_state_ != State::kIdle) {
    std::string error_msg{"bad stream state"};
    error_msg.append(", action: ").append(StreamActionToString(Action::kSendStartLine));
    error_msg.append(", stream state: ").append(StreamStateToString(write_state_));
    error_msg.append(", expected state: ").append(StreamStateToString(State::kIdle));
    TRPC_FMT_ERROR(error_msg);

    Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(error_msg)};
    OnError(status);
    return RetCode::kError;
  }

  NoncontiguousBufferBuilder builder;
  builder.Append(http::kHttpPrefix + status_line->version);
  builder.Append(" ");
  builder.Append(std::to_string(status_line->status_code));
  builder.Append(" ");
  builder.Append(status_line->status_text);
  builder.Append(trpc::http::kEmptyLine);
  *out = builder.DestructiveGet();

  // there is no need to transition state if status_code is 100-continue
  // expected that a response will be sent later
  if (status_line->status_code != 100) {
    write_state_ = State::kInit;
  }

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send start_line end", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id);
  return kSuccess;
}

}  // namespace trpc::stream
