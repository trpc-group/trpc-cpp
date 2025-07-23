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

#include <any>
#include <deque>
#include <memory>
#include <utility>

#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/session.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/stream/stream_handler.h"

namespace trpc::stream {

/// @brief The implementation of tRPC client stream handler.
class GrpcClientStreamHandler : public StreamHandler {
 public:
  explicit GrpcClientStreamHandler(StreamOptions&& options);
  ~GrpcClientStreamHandler() override = default;

  bool Init() override;

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override { return nullptr; }

  int RemoveStream(uint32_t stream_id) override { return -1; }

  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override { return -1; }

  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override {}

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override { return -1; }

  int ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) override;

  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  void SetSession(std::unique_ptr<http2::Session>&& session) { session_ = std::move(session); }
  http2::Session* GetSession() { return session_.get(); }

 protected:
  // @brief Submit the HTTP/2 request `request` and write the sendable data to `buffer`.
  int EncodeHttp2Request(const http2::RequestPtr& request, NoncontiguousBuffer* buffer);

  virtual int GetNetworkErrorCode() { return -1; }

 protected:
  StreamOptions options_;

  std::unique_ptr<http2::Session> session_{nullptr};

 private:
  // Store the stream ID corresponding to the unary call, so that when checking the response, it can be distinguished
  // between stream and unary packets. For unary packets, after checking, remove the ID from `unary_stream_ids`
  // (timeouts are still checked for response packets, so this set will not expand and cause OMM as long as the
  // connection exists).
  std::set<uint32_t> unary_stream_ids_;

  // Save the variables of the checked package in the session callback. In CheckMessage, the checked package will be
  // swapped.
  std::deque<std::any> out_;
};

/// @brief gRPC client stream protocol handler in separate/merge thread mode, currently mainly handling Unary RPC and
/// does not yet support streaming RPC.
class GrpcDefaultClientStreamHandler : public GrpcClientStreamHandler {
 public:
  explicit GrpcDefaultClientStreamHandler(StreamOptions&& options) : GrpcClientStreamHandler(std::move(options)) {}
  ~GrpcDefaultClientStreamHandler() override = default;

  int EncodeTransportMessage(IoMessage* msg) override;
};

/// @brief gRPC client stream protocol handler in Fiber mode, currently mainly handling Unary RPC and does not yet
/// support streaming RPC.
class GrpcFiberClientStreamHandler : public GrpcClientStreamHandler {
 public:
  explicit GrpcFiberClientStreamHandler(StreamOptions&& options) : GrpcClientStreamHandler(std::move(options)) {}
  ~GrpcFiberClientStreamHandler() override = default;

  int EncodeTransportMessage(IoMessage* msg) override;

  int ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) override;

 private:
  FiberMutex mutex_;
};

}  // namespace trpc::stream
