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

#include "trpc/stream/grpc/grpc_server_stream_connection_handler.h"

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/server_codec_factory.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/stream/grpc/util.h"
#include "trpc/stream/server_stream_handler_factory.h"
#include "trpc/stream/util.h"

namespace trpc::stream {

namespace internal {

void GrpcServerStreamConnectionHandler::Init(const BindInfo* bind_info, Connection* conn) {
  TRPC_ASSERT(bind_info && "bind_info is nullptr");
  TRPC_ASSERT(conn && "connection is nullptr");
  TRPC_ASSERT(conn->GetIoHandler() && "io_handler is nullptr");
  // Bind the callback used by gRPC to unpacking, mainly to determine whether to generate stream frames or unary frames.
  TRPC_ASSERT(bind_info->check_stream_rpc_function && "check_stream_rpc_function is nullptr");

  // Sets server_codec.
  // Codecs are only used when there are streaming interfaces.
  if (bind_info->has_stream_rpc) {
    server_codec_ = ServerCodecFactory::GetInstance()->Get(bind_info->protocol);
    TRPC_CHECK(server_codec_, "server codec is nullptr");
  }

  // Both gRPC streaming RPC and unary RPC require creating a StreamConnectionHandler.
  stream::StreamOptions stream_options{};
  stream_options.connection_id = conn->GetConnId();
  stream_options.server_mode = true;
  stream_options.fiber_mode = fiber_mode_;
  stream_options.check_stream_rpc_function = bind_info->check_stream_rpc_function;

  stream_handler_ = ServerStreamHandlerFactory::GetInstance()->Create(bind_info->protocol, std::move(stream_options));
  TRPC_CHECK(stream_handler_, "stream handler is nullptr");
}

int GrpcServerStreamConnectionHandler::DoHandshake(Connection* conn) {
  // Before the handshake, Connection's Send is not available, so a separate method is encapsulated here to write
  // data to the connection: WriteBufferToConnUntilDone.
  stream_handler_->GetMutableStreamOptions()->send = [this, conn](IoMessage&& msg) {
    return grpc::SendHandshakingMessage(conn->GetIoHandler(), std::move(msg.buffer));
  };
  if (!stream_handler_->Init()) {
    return -1;
  }
  // After the handshake, Connection is available, and sending uses Connection's Send function.
  stream_handler_->GetMutableStreamOptions()->send = [this, conn](IoMessage&& msg) {
    // Here we can ensure that all write operations on the stream are stopped before the connection is destroyed, so
    // we can use the connection directly to send data.
    return conn->Send(std::move(msg));
  };
  return 0;
}

int GrpcServerStreamConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                    std::deque<std::any>& out) {
  TRPC_FMT_TRACE("check grpc request, conn_id: {}, buffer size(pre): {}", conn->GetConnId(), in.ByteSize());
  if (TRPC_LIKELY(stream_handler_->ParseMessage(&in, &out) != 0)) {
    return PacketChecker::PACKET_ERR;
  }
  return !out.empty() ? PacketChecker::PACKET_FULL : PacketChecker::PACKET_LESS;
}

bool GrpcServerStreamConnectionHandler::EncodeStreamMessage(IoMessage* msg) {
  int encode_ok = stream_handler_->EncodeTransportMessage(msg);
  if (encode_ok != 0) {
    TRPC_FMT_ERROR("encode message for stream failed, error: {}", encode_ok);
    return false;
  }
  return true;
}

void GrpcServerStreamConnectionHandler::Stop() {
  if (stream_handler_) {
    stream_handler_->Stop();
  }
}

void GrpcServerStreamConnectionHandler::Join() {
  if (stream_handler_) {
    stream_handler_->Join();
  }
}

int GrpcServerStreamConnectionHandler::HandleStreamMessage(const BindInfo* bind_info, const ConnectionPtr& conn,
                                                           std::any& msg) {
  uint64_t recv_timestamp_us = trpc::time::GetMicroSeconds();
  // Pick the metadata of protocol message (frame header).
  std::any data = GrpcProtocolMessageMetadata{};
  if (!server_codec_->Pick(msg, data)) {
    return kStreamError;
  }

  auto& meta = std::any_cast<GrpcProtocolMessageMetadata&>(data);
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
  metadata.enable_stream = meta.enable_stream;
  metadata.stream_frame_type = meta.stream_frame_type;
  metadata.stream_id = meta.stream_id;

  stream_handler_->PushMessage(std::move(msg), std::move(metadata));
  return kStreamOk;
}

}  // namespace internal

}  // namespace trpc::stream
