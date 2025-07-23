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
#include "trpc/stream/stream_handler.h"
#include "trpc/transport/server/default/server_connection_handler.h"

namespace trpc::stream {

/// @brief Connection handler for http server in default thread model
class DefaultHttpServerStreamConnectionHandler : public DefaultServerStreamConnectionHandler {
 public:
  DefaultHttpServerStreamConnectionHandler(Connection* conn, BindAdapter* bind_adapter, BindInfo* bind_info)
      : DefaultServerStreamConnectionHandler(conn, bind_adapter, bind_info) {}

  void Init() override;

  void Stop() override;

  /// @brief Checks http messages
  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;

  /// @brief Handles stream messages.
  int HandleStreamMessage(const ConnectionPtr& conn, std::any& msg) override;

 protected:
  ServerCodecPtr server_codec_{nullptr};
  StreamHandlerPtr stream_handler_{nullptr};
};

}  // namespace trpc::stream
