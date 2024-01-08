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

#include "trpc/stream/trpc/trpc_client_stream_connection_handler.h"

#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/stream/client_stream_handler_factory.h"

namespace trpc::stream {

void FiberTrpcClientStreamConnectionHandler::Init() {
  TRPC_ASSERT(GetConnection() && "GetConnection() get nullptr");

  // It is not known whether the call includes streaming interfaces during `Init`, but the `client_codec_` can be
  // obtained from the factory to prevent repeated judgments and obtain non-NULL values later on.
  client_codec_ = ClientCodecFactory::GetInstance()->Get(GetTransInfo()->protocol);
  TRPC_ASSERT(client_codec_ && "trpc client codec not registered");

  // Create stream handler
  if (!stream_handler_) {
    StreamOptions options;
    options.server_mode = false;
    options.fiber_mode = true;
    options.connection_id = GetConnection()->GetConnId();
    options.send = [this](IoMessage&& message) {
      // Here, it can be guaranteed that all write operations on the stream have stopped before the connection is
      // destroyed, so it directly uses the `connection` to send.
      return GetConnection()->Send(std::move(message));
    };
    stream_handler_ = ClientStreamHandlerFactory::GetInstance()->Create(GetTransInfo()->protocol, std::move(options));
    stream_handler_->Init();
  }
}

StreamHandlerPtr FiberTrpcClientStreamConnectionHandler::GetOrCreateStreamHandler() {
  // When using fiber，stream_handler need to be created before.
  // Because with network connecting failed frequently, Stop/Join of this connection handler may be invoking before
  // GetOrCreateStreamHandler. This abnormal situation will cause Stop/Join of stream_handler not being invoked.
  use_stream_ = true;
  return stream_handler_;
}

bool FiberTrpcClientStreamConnectionHandler::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) {
  // Unary response.
  if (!use_stream_) {
    return FiberClientConnectionHandler::HandleMessage(conn, rsp_list);
  }

  // The interface contains a stream interface and needs to separate the unary and streaming packets of the stream.
  std::deque<std::any> unary_rsps;
  for (auto&& rsp_buf : rsp_list) {
    // Pick the metadata of protocol message (frame header).
    std::any data = TrpcProtocolMessageMetadata{};
    client_codec_->Pick(rsp_buf, data);
    auto& meta = std::any_cast<TrpcProtocolMessageMetadata&>(data);

    if (!meta.enable_stream) {
      // Unary response.
      unary_rsps.push_back(std::move(rsp_buf));
      continue;
    }

    ProtocolMessageMetadata metadata{};
    metadata.data_frame_type = meta.data_frame_type;
    metadata.enable_stream = meta.enable_stream;
    metadata.stream_frame_type = meta.stream_frame_type;
    metadata.stream_id = meta.stream_id;

    stream_handler_->PushMessage(std::move(rsp_buf), std::move(metadata));
  }

  if (!unary_rsps.empty()) {
    return FiberClientConnectionHandler::HandleMessage(conn, unary_rsps);
  }

  return true;
}

void FutureTrpcClientStreamConnComplexConnectionHandler::Init() {
  client_codec_ = ClientCodecFactory::GetInstance()->Get("trpc");
  TRPC_ASSERT(client_codec_ && "trpc client codec not registered");
}

StreamHandlerPtr FutureTrpcClientStreamConnComplexConnectionHandler::GetOrCreateStreamHandler() {
  if (!stream_handler_) {
    StreamOptions options;
    options.server_mode = false;
    options.fiber_mode = false;
    options.send = [this](IoMessage&& msg) { return GetConnection()->Send(std::move(msg)); };
    stream_handler_ = ClientStreamHandlerFactory::GetInstance()->Create("trpc", std::move(options));
    stream_handler_->Init();
  }
  return stream_handler_;
}

bool FutureTrpcClientStreamConnComplexConnectionHandler::HandleMessage(const ConnectionPtr& conn,
                                                                       std::deque<std::any>& rsp_list) {
  if (!stream_handler_) {
    return FutureClientStreamConnComplexConnectionHandler::HandleMessage(conn, rsp_list);
  }

  // The interface contains a stream interface and needs to separate the unary and streaming packets of the stream.
  std::deque<std::any> unary_rsps;
  for (auto& rsp_buf : rsp_list) {
    // Pick the metadata of protocol message (frame header).
    std::any data = TrpcProtocolMessageMetadata{};
    client_codec_->Pick(rsp_buf, data);
    auto& meta = std::any_cast<TrpcProtocolMessageMetadata&>(data);

    if (!meta.enable_stream) {
      // Unary response.
      unary_rsps.push_back(std::move(rsp_buf));
      continue;
    }

    ProtocolMessageMetadata metadata{};
    metadata.data_frame_type = meta.data_frame_type;
    metadata.enable_stream = meta.enable_stream;
    metadata.stream_frame_type = meta.stream_frame_type;
    metadata.stream_id = meta.stream_id;

    stream_handler_->PushMessage(std::move(rsp_buf), std::move(metadata));
  }

  if (!unary_rsps.empty()) {
    return FutureClientStreamConnComplexConnectionHandler::HandleMessage(conn, unary_rsps);
  }

  return true;
}

}  // namespace trpc::stream
