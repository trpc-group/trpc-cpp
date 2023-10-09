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

#include "trpc/stream/http/async/client/stream_connection_handler.h"

#include "trpc/stream/client_stream_handler_factory.h"

namespace trpc::stream {

StreamHandlerPtr HttpClientAsyncStreamConnectionHandler::GetOrCreateStreamHandler() {
  if (!stream_handler_) {
    StreamOptions options;
    options.server_mode = false;
    options.fiber_mode = false;
    options.send = [this](IoMessage&& msg) { return GetConnection()->Send(std::move(msg)); };
    options.connection_id = GetConnection()->GetConnId();
    stream_handler_ = ClientStreamHandlerFactory::GetInstance()->Create("http", std::move(options));
    stream_handler_->Init();
  }
  return stream_handler_;
}

int HttpClientAsyncStreamConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                         std::deque<std::any>& out) {
  if (!trans_info_->is_stream_proxy) {
    return FutureClientStreamConnPoolConnectionHandler::CheckMessage(conn, in, out);
  }
  int ret = stream_handler_->ParseMessage(&in, &out);
  if (TRPC_UNLIKELY(ret == -1)) {
    return PacketChecker::PACKET_ERR;
  }
  if (ret == -2 && out.empty()) {
    return PacketChecker::PACKET_LESS;
  }
  return PacketChecker::PACKET_FULL;
}

bool HttpClientAsyncStreamConnectionHandler::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msgs) {
  if (!trans_info_->is_stream_proxy) {
    return FutureClientStreamConnPoolConnectionHandler::HandleMessage(conn, msgs);
  }
  for (std::any& it : msgs) {
    stream_handler_->PushMessage(std::move(it), ProtocolMessageMetadata{});
  }
  return true;
}

}  // namespace trpc::stream
