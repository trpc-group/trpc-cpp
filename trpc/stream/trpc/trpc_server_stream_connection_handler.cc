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

#include "trpc/stream/trpc/trpc_server_stream_connection_handler.h"

#include "trpc/codec/server_codec_factory.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/stream/server_stream_handler_factory.h"
#include "trpc/stream/util.h"

namespace trpc::stream {

namespace internal {

void TrpcServerStreamConnectionHandler::Init(const BindInfo* bind_info, Connection* conn) {
  TRPC_ASSERT(bind_info && "bind_info is nullptr");
  TRPC_ASSERT(conn && "connection is nullptr");

  // StreamConnectionHandler is only needed by streaming RPC.
  if (!bind_info->has_stream_rpc) {
    return;
  }

  // Sets server_codec.
  server_codec_ = ServerCodecFactory::GetInstance()->Get(bind_info->protocol);
  TRPC_CHECK(server_codec_);

  // Creates stream handler.
  stream::StreamOptions options{};
  options.connection_id = conn->GetConnId();
  options.server_mode = true;
  options.fiber_mode = fiber_mode_;
  options.send = [this, conn](IoMessage&& msg) {
    // Here it can be ensured that all write operations on the stream are stopped before the connection is destroyed,
    // so you can directly use the connection to send.
    return conn->Send(std::move(msg));
  };

  stream_handler_ = ServerStreamHandlerFactory::GetInstance()->Create(bind_info->protocol, std::move(options));
  TRPC_CHECK(stream_handler_, "stream handler is nullptr");
  TRPC_CHECK(stream_handler_->Init(), "failed to initialize stream handler");
}

void TrpcServerStreamConnectionHandler::Stop() {
  if (stream_handler_) {
    stream_handler_->Stop();
  }
}

void TrpcServerStreamConnectionHandler::Join() {
  if (stream_handler_) {
    stream_handler_->Join();
  }
}

int TrpcServerStreamConnectionHandler::HandleStreamMessage(const BindInfo* bind_info, const ConnectionPtr& conn,
                                                           std::any& msg) {
  uint64_t recv_timestamp_us = trpc::time::GetMicroSeconds();
  // Pick the metadata of protocol message (frame header).
  std::any data = TrpcProtocolMessageMetadata{};
  if (!server_codec_->Pick(msg, data)) {
    return kStreamError;
  }

  auto& meta = std::any_cast<TrpcProtocolMessageMetadata&>(data);
  if (!meta.enable_stream) {
    return kStreamNonStreamMsg;
  }

  // Creates a stream if it was a new stream.
  if (stream_handler_->IsNewStream(meta.stream_id, meta.stream_frame_type)) {
    stream::StreamOptions stream_options;
    stream_options.stream_handler = stream_handler_;
    stream_options.stream_id = meta.stream_id;
    stream_options.connection_id = conn->GetConnId();
    stream_options.server_mode = true;
    stream_options.fiber_mode = fiber_mode_;
    stream_options.callbacks.handle_streaming_rpc_cb = bind_info->handle_streaming_rpc_function;
    stream_options.context.context = internal::BuildServerContext(conn, server_codec_, recv_timestamp_us);

    auto stream = stream_handler_->CreateStream(std::move(stream_options));
    if (stream == nullptr) {
      TRPC_FMT_ERROR("failed to create stream: {}", meta.stream_id);
    }
  }

  ProtocolMessageMetadata metadata{};
  metadata.data_frame_type = meta.data_frame_type;
  metadata.enable_stream = meta.enable_stream;
  metadata.stream_frame_type = meta.stream_frame_type;
  metadata.stream_id = meta.stream_id;

  stream_handler_->PushMessage(std::move(msg), std::move(metadata));
  return kStreamOk;
}

}  // namespace internal

}  // namespace trpc::stream
