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

#include "trpc/transport/server/fiber/fiber_server_connection_handler.h"

namespace trpc {

namespace stream {
bool FiberServerStreamConnectionHandler::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msgs) {
  auto* bind_info = GetBindInfo();
  if (!bind_info->has_stream_rpc) {
    return bind_info->msg_handle_function(conn, msgs);
  }

  std::deque<std::any> unary_msgs;
  for (std::any& it : msgs) {
    auto ret = HandleStreamMessage(conn, it);
    if (ret == kStreamOk) {
      continue;
    } else if (ret == kStreamNonStreamMsg) {
      unary_msgs.push_back(std::move(it));
      continue;
    } else if (ret == kStreamError) {
      return false;
    }
    // Unreachable.
    TRPC_ASSERT(false && "bad ret code");
  }

  if (!unary_msgs.empty()) {
    return bind_info->msg_handle_function(conn, unary_msgs);
  }

  return true;
}

}  // namespace stream

}  // namespace trpc
