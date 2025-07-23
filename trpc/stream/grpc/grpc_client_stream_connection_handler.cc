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

#include "trpc/stream/grpc/grpc_client_stream_connection_handler.h"

#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/grpc/util.h"

namespace trpc::stream {

namespace {

StreamHandlerPtr CreateAndInitStreamHandler(Connection* conn, bool fiber_mode) {
  StreamOptions options;
  options.fiber_mode = fiber_mode;
  options.connection_id = conn->GetConnId();
  StreamHandlerPtr stream_handler = ClientStreamHandlerFactory::GetInstance()->Create("grpc", std::move(options));
  return stream_handler;
}

bool EncodeStreamMessageHelper(const StreamHandlerPtr& stream_handler, IoMessage* msg) {
  // When `EncodeTransportMessage` is implemented internally by gRPC, it records the corresponding stream ID. This
  // serves as the basis for distinguishing between stream and unary packets when the server sends response messages.
  int encode_ok = stream_handler->EncodeTransportMessage(msg);
  if (encode_ok != 0) {
    TRPC_LOG_ERROR("encode message for stream failed, error: " << encode_ok);
    return false;
  }
  return true;
}

}  // namespace

int FiberGrpcClientStreamConnectionHandler::DoHandshake() {
  // Before the handshake, `Send` of the `Connection` is unavailable, so a separate method `WriteBufferToConnUntilDone`
  // is encapsulated here to write data to the file descriptor (`FD`).
  stream_handler_->GetMutableStreamOptions()->send = [this](IoMessage&& msg) {
    return grpc::SendHandshakingMessage(GetConnection()->GetIoHandler(), std::move(msg.buffer));
  };
  if (!stream_handler_->Init()) {
    return -1;
  }
  stream_handler_->GetMutableStreamOptions()->send = [this](IoMessage&& msg) {
    // It can be ensured here that all write operations on the stream have stopped before the connection is destroyed,
    // so it directly uses the `Connection` to send.
    return GetConnection()->Send(std::move(msg));
  };
  return 0;
}

void FiberGrpcClientStreamConnectionHandler::Init() {
  TRPC_ASSERT(GetConnection() && "GetConnection() get nullptr");
  TRPC_ASSERT(GetConnection()->GetIoHandler() && "IoHandler get nullptr");

  // It is not known whether the call includes stream interfaces during `Init`, but `client_codec_` can be obtained from
  // the factory to prevent repeated judgments and obtain non-NULL values later on.
  client_codec_ = ClientCodecFactory::GetInstance()->Get("grpc");
  TRPC_ASSERT(client_codec_ && "grpc client codec not registered");

  stream_handler_ = CreateAndInitStreamHandler(GetConnection(), true);
}

StreamHandlerPtr FiberGrpcClientStreamConnectionHandler::GetOrCreateStreamHandler() {
  TRPC_ASSERT(false && "grpc stream rpc for client is coming soon");
  has_stream_rpc_ = true;
  return stream_handler_;
}

int FiberGrpcClientStreamConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                         std::deque<std::any>& out) {
  TRPC_LOG_TRACE("check grpc request"
                 << ", connection id: " << conn->GetConnId() << ", buffer size(pre): " << in.ByteSize());

  if (TRPC_LIKELY(stream_handler_->ParseMessage(&in, &out) != 0)) {
    return PacketChecker::PACKET_ERR;
  }
  return !out.empty() ? PacketChecker::PACKET_FULL : PacketChecker::PACKET_LESS;
}

bool FiberGrpcClientStreamConnectionHandler::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) {
  if (!has_stream_rpc_) {
    return FiberClientConnectionHandler::HandleMessage(conn, rsp_list);
  }

  // The interface contains stream interfaces, which need to be separated into unary and streaming packets.
  std::deque<std::any> unary_rsps;
  for (auto&& rsp_buf : rsp_list) {
    std::any data = GrpcProtocolMessageMetadata{};
    client_codec_->Pick(rsp_buf, data);
    auto& meta = std::any_cast<GrpcProtocolMessageMetadata&>(data);
    if (!meta.enable_stream) {
      // Unary response.
      unary_rsps.push_back(std::move(rsp_buf));
      continue;
    }

    ProtocolMessageMetadata metadata{};
    metadata.enable_stream = meta.enable_stream;
    metadata.stream_frame_type = meta.stream_frame_type;
    metadata.stream_id = meta.stream_id;

    stream_handler_->PushMessage(std::move(rsp_buf), std::move(metadata));
  }

  if (!unary_rsps.empty()) {
    return FiberClientConnectionHandler::HandleMessage(conn, rsp_list);
  }

  return true;
}

bool FiberGrpcClientStreamConnectionHandler::EncodeStreamMessage(IoMessage* message) {
  return EncodeStreamMessageHelper(stream_handler_, message);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FutureGrpcClientStreamConnPoolConnectionHandler::DoHandshake() {
  // Before the handshake, `Send` of the `Connection` is unavailable, so a separate method `WriteBufferToConnUntilDone`
  // is encapsulated here to write data to the file descriptor (`FD`).
  stream_handler_->GetMutableStreamOptions()->send = [this](IoMessage&& msg) {
    return grpc::SendHandshakingMessage(GetConnection()->GetIoHandler(), std::move(msg.buffer));
  };
  if (!stream_handler_->Init()) {
    return -1;
  }
  stream_handler_->GetMutableStreamOptions()->send = [this](IoMessage&& msg) {
    // It can be ensured here that all write operations on the stream have stopped before the connection is destroyed,
    // so it directly uses the `Connection` to send.
    return GetConnection()->Send(std::move(msg));
  };
  return 0;
}

void FutureGrpcClientStreamConnPoolConnectionHandler::Init() {
  TRPC_ASSERT(GetConnection() && "GetConnection() get nullptr");
  TRPC_ASSERT(GetConnection()->GetIoHandler() && "IoHandler get nullptr");
  stream_handler_ = CreateAndInitStreamHandler(GetConnection(), false);
}

bool FutureGrpcClientStreamConnPoolConnectionHandler::EncodeStreamMessage(IoMessage* message) {
  return EncodeStreamMessageHelper(stream_handler_, message);
}

int FutureGrpcClientStreamConnPoolConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                                  std::deque<std::any>& out) {
  TRPC_LOG_TRACE("check grpc request"
                 << ", connection id: " << conn->GetConnId() << ", buffer size(pre): " << in.ByteSize());

  if (TRPC_LIKELY(stream_handler_->ParseMessage(&in, &out) != 0)) {
    return PacketChecker::PACKET_ERR;
  }
  return !out.empty() ? PacketChecker::PACKET_FULL : PacketChecker::PACKET_LESS;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int FutureGrpcClientStreamConnComplexConnectionHandler::DoHandshake() {
  // Before the handshake, `Send` of the `Connection` is unavailable, so a separate method `WriteBufferToConnUntilDone`
  // is encapsulated here to write data to the file descriptor (`FD`).
  stream_handler_->GetMutableStreamOptions()->send = [this](IoMessage&& msg) {
    return grpc::SendHandshakingMessage(GetConnection()->GetIoHandler(), std::move(msg.buffer));
  };
  if (!stream_handler_->Init()) {
    return -1;
  }
  stream_handler_->GetMutableStreamOptions()->send = [this](IoMessage&& msg) {
    // It can be ensured here that all write operations on the stream have stopped before the connection is destroyed,
    // so it directly uses the `Connection` to send.
    return GetConnection()->Send(std::move(msg));
  };
  return 0;
}

void FutureGrpcClientStreamConnComplexConnectionHandler::Init() {
  TRPC_ASSERT(GetConnection() && "GetConnection() get nullptr");
  TRPC_ASSERT(GetConnection()->GetIoHandler() && "IoHandler get nullptr");
  stream_handler_ = CreateAndInitStreamHandler(GetConnection(), false);
}

bool FutureGrpcClientStreamConnComplexConnectionHandler::EncodeStreamMessage(IoMessage* message) {
  return EncodeStreamMessageHelper(stream_handler_, message);
}

int FutureGrpcClientStreamConnComplexConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                                     std::deque<std::any>& out) {
  TRPC_LOG_TRACE("check grpc request"
                 << ", connection id: " << conn->GetConnId() << ", buffer size(pre): " << in.ByteSize());

  if (TRPC_LIKELY(stream_handler_->ParseMessage(&in, &out) != 0)) {
    return PacketChecker::PACKET_ERR;
  }
  return !out.empty() ? PacketChecker::PACKET_FULL : PacketChecker::PACKET_LESS;
}

}  // namespace trpc::stream
