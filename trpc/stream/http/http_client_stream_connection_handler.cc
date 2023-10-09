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

#include "trpc/stream/http/http_client_stream_connection_handler.h"

#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/http/http_client_stream_handler.h"
#include "trpc/util/http/stream/http_client_stream_response.h"
#include "trpc/util/log/logging.h"

namespace trpc::stream {

FiberHttpClientStreamConnectionHandler::~FiberHttpClientStreamConnectionHandler() {
  // Reset HTTP client stream object mounted on the connection, it's specific logic for HTTP synchronous stream.
  if (GetConnection()) {
    GetConnection()->SetUserAny(nullptr);
  }
}

void FiberHttpClientStreamConnectionHandler::Init() { TRPC_ASSERT(GetConnection() && "GetConnection() get nullptr"); }

StreamHandlerPtr FiberHttpClientStreamConnectionHandler::GetOrCreateStreamHandler() {
  std::unique_lock _(mutex_);
  // The `stream_handler` only needs to be created when a client calls the stream interface on this connection for the
  // first time. Whether the `stream_handler` is assigned can serve as a flag for distinguishing between streams and
  // unary calls.
  if (!stream_handler_) {
    StreamOptions options;
    options.fiber_mode = true;
    options.connection_id = GetConnection()->GetConnId();
    options.send = [this](IoMessage&& message) { return GetConnection()->Send(std::move(message)); };

    stream_handler_ = ClientStreamHandlerFactory::GetInstance()->Create(GetTransInfo()->protocol, std::move(options));
    stream_handler_->Init();
  }
  return stream_handler_;
}

int FiberHttpClientStreamConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                         std::deque<std::any>& out) {
  auto p = std::any_cast<HttpClientStreamResponsePtr>(&(conn->GetUserAny()));

  // Since stream_handler_ is gone, the stream object in the stream response should be recycled.
  if (!stream_handler_) {
    if (p) {
      (*p)->GetStream() = nullptr;
    }
    return FiberClientConnectionHandler::CheckMessage(conn, in, out);
  }

  // Since `stream_handler_` exists, construct a streaming response object to handle the message, and keep the `stream`
  // object in the response consistent with the `stream_handler_`.
  HttpClientStreamHandlerPtr handler_ptr = static_pointer_cast<HttpClientStreamHandler>(stream_handler_);
  if (p) {
    (*p)->GetStream() = handler_ptr->GetHttpStream();
    return FiberClientConnectionHandler::CheckMessage(conn, in, out);
  }

  auto response = std::make_shared<HttpClientStreamResponse>();
  response->GetStream() = handler_ptr->GetHttpStream();
  conn->SetUserAny(std::move(response));
  return FiberClientConnectionHandler::CheckMessage(conn, in, out);
}

bool FiberHttpClientStreamConnectionHandler::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) {
  // There is only one HTTP stream per connection, and streaming and unary calls cannot be mixed.
  if (stream_handler_ && static_pointer_cast<HttpClientStreamHandler>(stream_handler_)->GetHttpStream()) {
    for (auto&& rsp_buf : rsp_list) {
      stream_handler_->PushMessage(std::move(rsp_buf), ProtocolMessageMetadata{});
    }
    return true;
  }

  // The interface does not include stream interfaces and is directly processing unary packages.
  return FiberClientConnectionHandler::HandleMessage(conn, rsp_list);
}

void FiberHttpClientStreamConnectionHandler::CleanResource() {
  // Runs cleanup job for base class. e.g. Connection error will be set and prompted to caller.
  FiberClientStreamConnectionHandler::CleanResource();
  // Reset HTTP client stream object mounted on the connection, it's specific logic for HTTP synchronous stream.
  if (GetConnection()) {
    GetConnection()->SetUserAny(nullptr);
  }
}

}  // namespace trpc::stream
