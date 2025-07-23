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

#include "trpc/stream/grpc/grpc_client_stream_handler.h"

#include <mutex>

#include "trpc/client/client_context.h"
#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/http2/client_session.h"
#include "trpc/codec/grpc/http2/response.h"

namespace trpc::stream {

GrpcClientStreamHandler::GrpcClientStreamHandler(StreamOptions&& options) : options_(std::move(options)) {
  session_ = std::make_unique<http2::ClientSession>(http2::SessionOptions());
}

bool GrpcClientStreamHandler::Init() {
  // When initialized, send the client HTTP/2 Preface message.
  NoncontiguousBuffer buffer;
  auto signal_ok = session_->SignalWrite(&buffer);
  if (signal_ok < 0) {
    TRPC_LOG_ERROR("signal write request failed, error: " << signal_ok);
    return false;
  }

  IoMessage io_msg;
  io_msg.buffer = std::move(buffer);

  if (options_.send(std::move(io_msg)) != 0) {
    TRPC_LOG_ERROR("send client http2 preface failed");
    return false;
  }

  session_->SetOnResponseCallback([this](http2::ResponsePtr&& response) {
    auto it = unary_stream_ids_.find(response->GetStreamId());
    if (it != unary_stream_ids_.end()) {
      unary_stream_ids_.erase(it);
    }
    out_.emplace_back(std::move(response));
  });

  return true;
}

int GrpcClientStreamHandler::ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  if (session_->SignalRead(in) != 0) {
    TRPC_LOG_ERROR("parse message failed(client)");
    return -1;
  }
  out->swap(out_);
  return 0;
}

int GrpcClientStreamHandler::EncodeHttp2Request(const http2::RequestPtr& request, NoncontiguousBuffer* buffer) {
  int submit_ok = session_->SubmitRequest(request);
  if (submit_ok != 0) {
    TRPC_LOG_ERROR("submit request failed, error: " << submit_ok);
    return submit_ok;
  }

  int signal_ok = session_->SignalWrite(buffer);
  if (signal_ok < 0) {
    TRPC_LOG_ERROR("signal write request failed, error: " << signal_ok);
    return signal_ok;
  }
  unary_stream_ids_.insert(request->GetStreamId());
  return 0;
}

namespace {
http2::RequestPtr GetHttp2Request(const std::any& msg) {
  http2::RequestPtr http2_request{nullptr};
  try {
    const auto& context = std::any_cast<const ClientContextPtr&>(msg);
    auto grpc_unary_request = static_cast<GrpcUnaryRequestProtocol*>(context->GetRequest().get());
    http2_request = grpc_unary_request->GetHttp2Request();
  } catch (std::bad_any_cast& e) {
    TRPC_LOG_ERROR("exception: " << e.what() << ", " << msg.type().name());
  }
  return http2_request;
}
}  // namespace

int GrpcDefaultClientStreamHandler::EncodeTransportMessage(IoMessage* msg) {
  http2::RequestPtr http2_request = GetHttp2Request(msg->msg);
  if (TRPC_UNLIKELY(!http2_request)) {
    return -1;
  }

  if (TRPC_UNLIKELY(EncodeHttp2Request(http2_request, &(msg->buffer)) != 0)) {
    return -1;
  }

  return 0;
}

int GrpcFiberClientStreamHandler::EncodeTransportMessage(IoMessage* msg) {
  http2::RequestPtr http2_request = GetHttp2Request(msg->msg);
  if (TRPC_UNLIKELY(!http2_request)) {
    return -1;
  }

  {
    std::unique_lock lock(mutex_);
    NoncontiguousBuffer buffer;
    if (TRPC_UNLIKELY(EncodeHttp2Request(http2_request, &buffer) != 0)) {
      return -1;
    }
    // The http2 RFC requires that the `stream_id` corresponding to the first frame (`HEADER`) of a client's send stream
    // be sequential. Under Fiber, multiple threads send different stream packets on the same connection, which can
    // cause packet chaos (this issue does not occur in separate/merge mode). Therefore, locking protection is needed
    // during the sending phase.
    IoMessage io_msg;
    io_msg.buffer = std::move(buffer);
    if (options_.send(std::move(io_msg)) != 0) {
      TRPC_LOG_ERROR("send http2 response failed");
      return -1;
    }
  }
  return 0;
}

int GrpcFiberClientStreamHandler::ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  std::unique_lock lock(mutex_);
  return GrpcClientStreamHandler::ParseMessage(in, out);
}

}  // namespace trpc::stream
