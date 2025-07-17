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

#include "trpc/codec/client_codec.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"
#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler.h"
#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler.h"

namespace trpc::stream {

/// @brief Implementation of stream connection handler for gRPC client stream which works in FIBER thread model.
class FiberGrpcClientStreamConnectionHandler : public FiberClientStreamConnectionHandler {
 public:
  explicit FiberGrpcClientStreamConnectionHandler(Connection* conn, TransInfo* trans_info)
      : FiberClientStreamConnectionHandler(conn, trans_info) {}

  void Init() override;
  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override;
  StreamHandlerPtr GetOrCreateStreamHandler() override;

  void Stop() override {
    if (has_stream_rpc_) {
      stream_handler_->Stop();
    }
  }

  void Join() override {
    if (has_stream_rpc_) {
      stream_handler_->Join();
    }
  }

  // Note: gRPC's unary RPC requires stateful encoding of messages before sending them.
  bool EncodeStreamMessage(IoMessage* message) override;
  // @brief Whether to send the encoded stream messages at the network layer.
  bool NeedSendStreamMessage() override { return false; }
  int DoHandshake() override;

 private:
  /// Used to distinguish between streaming and non-streaming packets in mixed packet scenarios.
  ClientCodecPtr client_codec_{nullptr};
  StreamHandlerPtr stream_handler_{nullptr};
  // gRPC flag used to determine if it includes stream RPC.
  bool has_stream_rpc_{false};
};

/// @brief Implementation of stream connection handler for tRPC client stream which works in [SEPARATE/MERGE] thread
/// model.
class FutureGrpcClientStreamConnPoolConnectionHandler : public FutureClientStreamConnPoolConnectionHandler {
 public:
  FutureGrpcClientStreamConnPoolConnectionHandler(const FutureConnectorOptions& options,
                                                  FutureConnPoolMessageTimeoutHandler& msg_timeout_handler)
      : FutureClientStreamConnPoolConnectionHandler(options, msg_timeout_handler) {}

  void Init() override;
  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;
  bool EncodeStreamMessage(IoMessage* message) override;
  int DoHandshake() override;

 private:
  StreamHandlerPtr stream_handler_{nullptr};
};

/// @brief Implementation of stream connection handler for gRPC client stream which works in FIBER thread model.
class FutureGrpcClientStreamConnComplexConnectionHandler : public FutureClientStreamConnComplexConnectionHandler {
 public:
  FutureGrpcClientStreamConnComplexConnectionHandler(const FutureConnectorOptions& options,
                                                     FutureConnComplexMessageTimeoutHandler& msg_timeout_handler)
      : FutureClientStreamConnComplexConnectionHandler(options, msg_timeout_handler) {}

  void Init() override;
  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;
  bool EncodeStreamMessage(IoMessage* message) override;
  int DoHandshake() override;

 private:
  StreamHandlerPtr stream_handler_{nullptr};
};

}  // namespace trpc::stream
