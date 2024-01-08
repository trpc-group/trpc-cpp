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

#include "trpc/codec/client_codec.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"
#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler.h"
#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler.h"

namespace trpc::stream {

/// @brief Implementation of stream connection handler for tRPC client stream which works in FIBER thread model.
class FiberTrpcClientStreamConnectionHandler : public FiberClientStreamConnectionHandler {
 public:
  explicit FiberTrpcClientStreamConnectionHandler(Connection* conn, TransInfo* trans_info)
      : FiberClientStreamConnectionHandler(conn, trans_info) {}

  void Init() override;
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override;
  StreamHandlerPtr GetOrCreateStreamHandler() override;
  void Stop() override {
    if (stream_handler_) {
      stream_handler_->Stop();
    }
  }

  void Join() override {
    if (stream_handler_) {
      stream_handler_->Join();
    }
  }

 private:
  /// Used to distinguish between streaming and non-streaming packets in mixed packet scenarios.
  ClientCodecPtr client_codec_{nullptr};
  StreamHandlerPtr stream_handler_{nullptr};
  /// Flag used to avoids connection MessageHandle doing stream frame distrubution when stream is not needed.
  /// Default to false, true if GetOrCreateStreamHandler being invoked.
  bool use_stream_{false};
};

/// @brief Implementation of stream connection handler for tRPC client stream which works in [SEPARATE/MERGE] thread
/// model.
class FutureTrpcClientStreamConnComplexConnectionHandler : public FutureClientStreamConnComplexConnectionHandler {
 public:
  FutureTrpcClientStreamConnComplexConnectionHandler(const FutureConnectorOptions& options,
                                                     FutureConnComplexMessageTimeoutHandler& msg_timeout_handler)
      : FutureClientStreamConnComplexConnectionHandler(options, msg_timeout_handler) {}

  void Init() override;
  StreamHandlerPtr GetOrCreateStreamHandler() override;
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) override;
  void Stop() override {
    // Only stream RPC requires stream cleanup, unary does not.
    if (stream_handler_) {
      stream_handler_->Stop();
    }
  }
  void Join() override {
    // Only stream RPC requires stream cleanup, unary does not.
    if (stream_handler_) {
      stream_handler_->Join();
    }
  }

 protected:
  ClientCodecPtr client_codec_{nullptr};
  StreamHandlerPtr stream_handler_{nullptr};
};

}  // namespace trpc::stream
