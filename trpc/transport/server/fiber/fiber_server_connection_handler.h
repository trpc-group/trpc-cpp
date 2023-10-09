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
#include "trpc/transport/server/fiber/fiber_bind_adapter.h"
#include "trpc/util/function.h"

namespace trpc {

using FiberServerConnectionHandler = ServerConnectionHandler<FiberBindAdapter>;

/// @brief The function of create fiber connection handler
using FiberServerConnectionHandlerCreator =
    Function<std::unique_ptr<FiberServerConnectionHandler>(Connection*, FiberBindAdapter*, BindInfo*)>;

namespace stream {

/// @brief Connection handler fo streaming protocol.
class FiberServerStreamConnectionHandler : public FiberServerConnectionHandler {
 public:
  FiberServerStreamConnectionHandler(Connection* conn, FiberBindAdapter* bind_adapter, BindInfo* bind_info)
      : FiberServerConnectionHandler(conn, bind_adapter, bind_info) {}

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
