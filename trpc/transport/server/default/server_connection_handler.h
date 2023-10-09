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

#include <memory>

#include "trpc/codec/server_codec.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/transport/server/common/server_connection_handler.h"
#include "trpc/transport/server/default/bind_adapter.h"
#include "trpc/util/function.h"

namespace trpc {

using DefaultServerConnectionHandler = ServerConnectionHandler<BindAdapter>;

/// @brief The function for creating a default connection handler
using DefaultServerConnectionHandlerCreator =
    Function<std::unique_ptr<DefaultServerConnectionHandler>(Connection*, BindAdapter*, BindInfo*)>;

namespace stream {

/// @brief Connection handler fo streaming protocol.
class DefaultServerStreamConnectionHandler : public DefaultServerConnectionHandler {
 public:
  DefaultServerStreamConnectionHandler(Connection* conn, BindAdapter* bind_adapter, BindInfo* bind_info)
      : DefaultServerConnectionHandler(conn, bind_adapter, bind_info) {}

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msgs) override;

  /// @brief Handles stream messages.
  ///
  /// @param conn is the channel which message read from.
  /// @param msg is a stream message.
  /// @return Returns dispatch result, following values may occur:
  /// kStreamOk, processes successfully.
  /// kStreamError, an critical error occurred.
  /// kStreamNonStreamMsg, input |msg| isn't a stream message.
  virtual int HandleStreamMessage(const ConnectionPtr& conn, std::any& msg) = 0;
};

}  // namespace stream

}  // namespace trpc
