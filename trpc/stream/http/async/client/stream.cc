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

#include "trpc/stream/http/async/client/stream.h"

#include "trpc/util/http/common.h"
#include "trpc/util/http/response.h"

namespace trpc::stream {

Status HttpClientAsyncStream::Finish() {
  if (!IsStateTerminate()) {
    Status err_status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream not close"};
    TRPC_FMT_ERROR("stream not close, read_state_: {}, write_state_:{}", StreamStateToString(read_state_),
                   StreamStateToString(write_state_));
    OnError(err_status);
  }
  auto& context = std::any_cast<ClientContextPtr&>(GetMutableStreamOptions()->context.context);
  filter_controller_->RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);
  filter_controller_->RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  Stop();
  GetMutableStreamOptions()->callbacks.on_close_cb(GetStatus().GetFrameworkRetCode());
  return GetStatus();
}

void HttpClientAsyncStream::Reset(Status status) {
  OnError(status);
  auto& context = std::any_cast<ClientContextPtr&>(GetMutableStreamOptions()->context.context);
  filter_controller_->RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);
  filter_controller_->RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  Stop();
  GetMutableStreamOptions()->callbacks.on_close_cb(status.GetFrameworkRetCode());
}

void HttpClientAsyncStream::OnError(Status status) {
  if (!GetStatus().OK()) {
    return;
  }
  if (pending_status_line_.val) {
    NotifyPending(&pending_status_line_, std::move(status_line_));
    status_line_.reset();
  }
  HttpAsyncStream::OnError(status);
}

RetCode HttpClientAsyncStream::HandleRecvMessage(HttpStreamFramePtr&& msg) {
  switch (msg->GetFrameType()) {
    case HttpStreamFrame::HttpStreamFrameType::kStatusLine: {
      auto frame = static_cast<HttpStreamStatusLine*>(msg.Get());
      return HandleStatusLine(frame->GetMutableHttpStatusLine());
    }
    default: {
      return HttpAsyncStream::HandleRecvMessage(std::move(msg));
    }
  }
}

RetCode HttpClientAsyncStream::HandleStatusLine(HttpStatusLine* status_line) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle start_line begin, {} {} {}", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id, status_line->version, status_line->status_code,
                 status_line->status_text);

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

  if (!pending_status_line_.val) {
    status_line_ = std::make_optional<HttpStatusLine>(std::move(*status_line));
  } else {
    NotifyPending(&pending_status_line_, std::make_optional<HttpStatusLine>(std::move(*status_line)));
  }

  // If it is `100-continue`, there is no need to change the stream status, and it is expected that there will be a
  // subsequent response.
  if (status_line->status_code != 100) {
    read_state_ = State::kInit;
  }

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, handle start_line end", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id);

  return kSuccess;
}

Future<HttpStatusLine> HttpClientAsyncStream::ReadStatusLine(int timeout) {
  pending_status_line_.val = Promise<HttpStatusLine>();

  auto ft = pending_status_line_.val.value().GetFuture();

  if (status_line_.has_value()) {
    NotifyPending(&pending_status_line_, std::move(status_line_));
    status_line_.reset();
  }

  if (timeout == std::numeric_limits<int>::max() || ft.IsReady() || ft.IsFailed()) {
    return ft;
  }

  CreatePendingTimer(&pending_status_line_, timeout);

  return ft;
}

RetCode HttpClientAsyncStream::HandleSendMessage(HttpStreamFramePtr&& msg, NoncontiguousBuffer* out) {
  switch (msg->GetFrameType()) {
    case HttpStreamFrame::HttpStreamFrameType::kRequestLine: {
      auto frame = static_cast<HttpStreamRequestLine*>(msg.Get());
      return PreSendRequestLine(frame->GetMutableHttpRequestLine(), out);
    }
    default: {
      return HttpAsyncStream::HandleSendMessage(std::move(msg), out);
    }
  }
}

RetCode HttpClientAsyncStream::PreSendRequestLine(HttpRequestLine* start_line, NoncontiguousBuffer* out) {
  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send start_line begin, {} {} {}", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id, start_line->method, start_line->request_uri,
                 start_line->version);

  if (write_state_ != State::kIdle) {
    std::string error_msg{"bad stream state"};
    error_msg.append(", action: ").append(StreamActionToString(Action::kSendStartLine));
    error_msg.append(", stream state: ").append(StreamStateToString(write_state_));
    error_msg.append(", expected state: ").append(StreamStateToString(State::kIdle));
    TRPC_FMT_ERROR(error_msg);

    Status status{trpc::http::HttpResponse::StatusCode::kInternalServerError, 0, std::move(error_msg)};
    OnError(status);
    return kError;
  }

  NoncontiguousBufferBuilder builder;
  builder.Append(start_line->method);
  builder.Append(" ");
  builder.Append(start_line->request_uri);
  builder.Append(" ");
  builder.Append(http::kHttpPrefix + start_line->version);
  builder.Append(trpc::http::kEmptyLine);
  *out = builder.DestructiveGet();

  write_state_ = State::kInit;

  TRPC_FMT_DEBUG("server: {}, conn_id: {}, send start_line end", GetMutableStreamOptions()->server_mode,
                 GetMutableStreamOptions()->connection_id);

  return kSuccess;
}

}  // namespace trpc::stream
