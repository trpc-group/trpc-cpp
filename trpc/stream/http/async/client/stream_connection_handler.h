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

#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler.h"

namespace trpc::stream {

/// @brief The class for handling the HTTP stream protocol on the client-connected connection in separate/merge thread
/// mode with the connection pooling pattern.
class HttpClientAsyncStreamConnectionHandler : public FutureClientStreamConnPoolConnectionHandler {
 public:
  HttpClientAsyncStreamConnectionHandler(const FutureConnectorOptions& options,
                                         FutureConnPoolMessageTimeoutHandler& msg_timeout_handler)
      : FutureClientStreamConnPoolConnectionHandler(options, msg_timeout_handler) {
    trans_info_ = options.group_options->trans_info;
  }

  void Init() override {
    TRPC_ASSERT(GetConnection() && "GetConnection() get nullptr");
    TRPC_ASSERT(trans_info_ && "transinfo get nullptr");
  }

  StreamHandlerPtr GetOrCreateStreamHandler() override;

  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override;

 private:
  StreamHandlerPtr stream_handler_{nullptr};
  TransInfo* trans_info_{nullptr};
};

}  // namespace trpc::stream
