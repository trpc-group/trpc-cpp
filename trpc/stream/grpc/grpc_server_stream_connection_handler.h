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

#include "trpc/codec/server_codec.h"
#include "trpc/transport/server/default/server_connection_handler.h"
#include "trpc/transport/server/fiber/fiber_server_connection_handler.h"

namespace trpc::stream {

namespace internal {
class GrpcServerStreamConnectionHandler {
 public:
  explicit GrpcServerStreamConnectionHandler(bool fiber_mode) : fiber_mode_(fiber_mode) {}

  void Init(const BindInfo* bind_info, Connection* conn);
  void Stop();
  void Join();
  int HandleStreamMessage(const BindInfo* bind_info, const ConnectionPtr& conn, std::any& msg);
  int DoHandshake(Connection* conn);
  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out);
  bool EncodeStreamMessage(IoMessage* msg);

 private:
  bool fiber_mode_{false};
  ServerCodecPtr server_codec_{nullptr};
  StreamHandlerPtr stream_handler_{nullptr};
};

}  // namespace internal

/// @brief Implementation of stream connection handler for gRPC server stream which works in FIBER thread model.
class FiberGrpcServerStreamConnectionHandler : public FiberServerStreamConnectionHandler {
 public:
  FiberGrpcServerStreamConnectionHandler(Connection* conn, FiberBindAdapter* bind_adapter, BindInfo* bind_info)
      : FiberServerStreamConnectionHandler(conn, bind_adapter, bind_info) {}

  void Init() override { handler_.Init(GetBindInfo(), GetConnection()); }
  void Stop() override { handler_.Stop(); }
  void Join() override { handler_.Join(); }
  int HandleStreamMessage(const ConnectionPtr& conn, std::any& msg) override {
    return handler_.HandleStreamMessage(GetBindInfo(), conn, msg);
  }
  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    return handler_.CheckMessage(conn, in, out);
  }
  int DoHandshake() override { return handler_.DoHandshake(GetConnection()); }
  bool EncodeStreamMessage(IoMessage* msg) override { return handler_.EncodeStreamMessage(msg); }

 private:
  internal::GrpcServerStreamConnectionHandler handler_{true};
};

/// @brief Implementation of stream connection handler for gRPC server stream which works in DEFAULT thread model.
class DefaultGrpcServerStreamConnectionHandler : public DefaultServerStreamConnectionHandler {
 public:
  DefaultGrpcServerStreamConnectionHandler(Connection* conn, BindAdapter* bind_adapter, BindInfo* bind_info)
      : DefaultServerStreamConnectionHandler(conn, bind_adapter, bind_info) {}

  void Init() override { handler_.Init(GetBindInfo(), GetConnection()); }
  void Stop() override { handler_.Stop(); }
  void Join() override { handler_.Join(); }
  int HandleStreamMessage(const ConnectionPtr& conn, std::any& msg) override {
    return handler_.HandleStreamMessage(GetBindInfo(), conn, msg);
  }
  int DoHandshake() override { return handler_.DoHandshake(GetConnection()); }
  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    return handler_.CheckMessage(conn, in, out);
  }
  bool EncodeStreamMessage(IoMessage* msg) override { return handler_.EncodeStreamMessage(msg); }

 private:
  internal::GrpcServerStreamConnectionHandler handler_{false};
};

}  // namespace trpc::stream
