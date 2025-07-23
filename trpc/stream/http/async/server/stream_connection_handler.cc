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

#include "trpc/stream/http/async/server/stream_connection_handler.h"

#include "trpc/codec/server_codec_factory.h"
#include "trpc/stream/server_stream_handler_factory.h"
#include "trpc/stream/util.h"

namespace trpc::stream {

void DefaultHttpServerStreamConnectionHandler::Init() {
  auto bind_info = GetBindInfo();
  auto conn = GetConnection();
  TRPC_ASSERT(bind_info && "bind_info is nullptr");
  TRPC_ASSERT(conn && "connection is nullptr");

  // Sets server_codec.
  server_codec_ = ServerCodecFactory::GetInstance()->Get(bind_info->protocol);
  TRPC_CHECK(server_codec_);

  // Creates stream handler.
  StreamOptions options;
  options.connection_id = conn->GetConnId();
  options.server_mode = true;
  options.fiber_mode = false;
  options.send = [this, conn](IoMessage&& msg) { return conn->Send(std::move(msg)); };

  stream_handler_ = ServerStreamHandlerFactory::GetInstance()->Create(bind_info->protocol, std::move(options));
  TRPC_CHECK(stream_handler_, "stream handler is nullptr");
  TRPC_CHECK(stream_handler_->Init(), "failed to initialize stream handler");
}

void DefaultHttpServerStreamConnectionHandler::Stop() {
  if (stream_handler_) {
    stream_handler_->Stop();
  }
}

int DefaultHttpServerStreamConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                           std::deque<std::any>& out) {
  if (!GetBindInfo()->has_stream_rpc) {
    return DefaultServerStreamConnectionHandler::CheckMessage(conn, in, out);
  }
  int ret = stream_handler_->ParseMessage(&in, &out);
  if (TRPC_UNLIKELY(ret == -1)) {
    return trpc::kPacketError;
  }
  if (ret == -2 && out.empty()) {
    return trpc::kPacketLess;
  }
  return trpc::kPacketFull;
}

int DefaultHttpServerStreamConnectionHandler::HandleStreamMessage(const ConnectionPtr& conn, std::any& msg) {
  uint64_t recv_timestamp_us = trpc::time::GetMicroSeconds();

  // Creates a stream if it was a new stream.
  ProtocolMessageMetadata meta;
  if (stream_handler_->IsNewStream(meta.stream_id, meta.stream_frame_type)) {
    stream::StreamOptions stream_options;
    stream_options.stream_handler = stream_handler_;
    stream_options.stream_id = meta.stream_id;
    stream_options.connection_id = conn->GetConnId();
    stream_options.server_mode = true;
    stream_options.callbacks.handle_streaming_rpc_cb = GetBindInfo()->handle_streaming_rpc_function;
    stream_options.context.context = internal::BuildServerContext(conn, server_codec_, recv_timestamp_us);

    stream_handler_->CreateStream(std::move(stream_options));
  }

  stream_handler_->PushMessage(std::move(msg), std::move(meta));
  return kStreamOk;
}

}  // namespace trpc::stream
