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

#pragma once

#include "trpc/codec/server_codec.h"
#include "trpc/transport/server/default/server_connection_handler.h"
#include "trpc/transport/server/fiber/fiber_server_connection_handler.h"

namespace trpc::stream {

namespace internal {

class TrpcServerStreamConnectionHandler {
 public:
  explicit TrpcServerStreamConnectionHandler(bool fiber_mode) : fiber_mode_(fiber_mode) {}

  void Init(const BindInfo* bind_info, Connection* conn);
  void Stop();
  void Join();
  int HandleStreamMessage(const BindInfo* bind_info, const ConnectionPtr& conn, std::any& msg);

 private:
  bool fiber_mode_{false};
  ServerCodecPtr server_codec_{nullptr};
  StreamHandlerPtr stream_handler_{nullptr};
};

}  // namespace internal

/// @brief Implementation of stream connection handler for tRPC server stream which works in FIBER thread model.
class FiberTrpcServerStreamConnectionHandler : public FiberServerStreamConnectionHandler {
 public:
  FiberTrpcServerStreamConnectionHandler(Connection* conn, FiberBindAdapter* bind_adapter, BindInfo* bind_info)
      : FiberServerStreamConnectionHandler(conn, bind_adapter, bind_info) {}

  void Init() override { handler_.Init(GetBindInfo(), GetConnection()); }
  void Stop() override { handler_.Stop(); }
  void Join() override { handler_.Join(); }
  int HandleStreamMessage(const ConnectionPtr& conn, std::any& msg) override {
    return handler_.HandleStreamMessage(GetBindInfo(), conn, msg);
  }

 private:
  internal::TrpcServerStreamConnectionHandler handler_{true};
};

/// @brief Implementation of stream connection handler for tRPC server stream which works in DEFAULT thread model.
class DefaultTrpcServerStreamConnectionHandler : public DefaultServerStreamConnectionHandler {
 public:
  DefaultTrpcServerStreamConnectionHandler(Connection* conn, BindAdapter* bind_adapter, BindInfo* bind_info)
      : DefaultServerStreamConnectionHandler(conn, bind_adapter, bind_info) {}

  void Init() override { handler_.Init(GetBindInfo(), GetConnection()); }
  void Stop() override { handler_.Stop(); }
  void Join() override { handler_.Join(); }
  int HandleStreamMessage(const ConnectionPtr& conn, std::any& msg) override {
    return handler_.HandleStreamMessage(GetBindInfo(), conn, msg);
  }

 private:
  internal::TrpcServerStreamConnectionHandler handler_{false};
};

}  // namespace trpc::stream
