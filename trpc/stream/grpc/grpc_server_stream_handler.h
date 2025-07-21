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

#pragma once

#include "trpc/codec/grpc/http2/response.h"
#include "trpc/codec/grpc/http2/session.h"
#include "trpc/codec/server_codec_factory.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/filter/server_filter_controller.h"
#include "trpc/stream/grpc/grpc_stream.h"
#include "trpc/stream/stream_handler.h"

namespace trpc::stream {

/// @brief Implementation of stream handler for gRPC server stream.
class GrpcServerStreamHandler : public StreamHandler {
 public:
  explicit GrpcServerStreamHandler(StreamOptions&& options);
  ~GrpcServerStreamHandler() = default;

  bool Init() override;

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override { return nullptr; }

  int RemoveStream(uint32_t stream_id) override { return -1; }

  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override { return -1; }

  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override {}

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override { return -1; }

  int ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) override;

  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  // @brief Gets or sets HTTP2 session.
  void SetSession(std::unique_ptr<http2::Session>&& session) { session_ = std::move(session); }
  http2::Session* GetSession() { return session_.get(); }

 protected:
  // @brief Submit the HTTP/2 response and write the available data to the buffer.
  int EncodeHttp2Response(const http2::ResponsePtr& response, NoncontiguousBuffer* buffer);

  virtual int GetNetworkErrorCode() { return -1; }

 protected:
  StreamOptions options_;
  // HTTP/2 session that manages HTTP/2 streams and protocol encoding/decoding.
  std::unique_ptr<http2::Session> session_{nullptr};

 private:
  // Save the variables of the checked package in the session callback. In CheckMessage, the checked package will be
  // swapped.
  std::deque<std::any> out_;
};

/// @brief gRPC server stream protocol handler under separation/merging thread mode, currently mainly handling unary
/// RPC, streaming RPC is not supported for now.
class DefaultGrpcServerStreamHandler : public GrpcServerStreamHandler {
 public:
  explicit DefaultGrpcServerStreamHandler(StreamOptions&& options) : GrpcServerStreamHandler(std::move(options)) {}
  ~DefaultGrpcServerStreamHandler() = default;

  int EncodeTransportMessage(IoMessage* msg) override;
};

/// @brief gRPC server stream protocol handler under Fiber mode, currently mainly handling unary RPC, and supporting
/// streaming RPC.
class FiberGrpcServerStreamHandler : public GrpcServerStreamHandler {
 public:
  explicit FiberGrpcServerStreamHandler(StreamOptions&& options) : GrpcServerStreamHandler(std::move(options)) {
    server_codec_ = ServerCodecFactory::GetInstance()->Get("grpc");
    if (!server_codec_) {
      TRPC_LOG_ERROR("failed to get server_codec protocol: grpc");
      TRPC_ASSERT(server_codec_);
    }
  }
  ~FiberGrpcServerStreamHandler() = default;

  bool Init() override;

  void Stop() override;

  void Join() override;

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override;

  int RemoveStream(uint32_t stream_id) override;

  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override;

  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override;

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override;

  int ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) override;

  int EncodeTransportMessage(IoMessage* msg) override;

 private:
  void Clear(const Status& status);

  int GetNetworkErrorCode() override;

  StreamRecvMessage CreateResetMessage(uint32_t stream_id, uint32_t error_code);

 private:
  // Used to manage mutex of session write operations.
  FiberMutex session_mutex_;
  // Used for session-level flow control.
  FiberConditionVariable session_cv_;
  // Used to manage mutex of streams.
  FiberMutex stream_mutex_;
  // List of streams attached to the connection.
  std::unordered_map<uint32_t, GrpcStreamPtr> streams_;
  ServerCodecPtr server_codec_;
  // Flag indicating whether the connection has ended. When the connection ends, only the normal termination of
  // existing streams will be waited for, and no new streams or stream frames will be pushed.
  bool conn_closed_{false};
};

}  // namespace trpc::stream
