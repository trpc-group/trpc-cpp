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

#include "trpc/runtime/iomodel/reactor/common/accept_connection_info.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/event_handler.h"

namespace trpc {

/// @brief Base class for accept connection
class Acceptor : public EventHandler {
 public:
  /// @brief Enable listen connection
  virtual bool EnableListen(int backlog = 1024) = 0;

  /// @brief Disable listen connection
  virtual void DisableListen() = 0;

  /// @brief Set the function of proccessing connection after the server accepts
  void SetAcceptHandleFunction(AcceptConnectionFunction&& func) { accept_handler_ = std::move(func); }

  /// @brief Get the function of proccessing connection after the server accepts
  AcceptConnectionFunction& GetAcceptHandleFunction() { return accept_handler_; }

  /// @brief Set the function of set socket option after the server accepts the connection
  void SetAcceptSetSocketOptFunction(const SetSocketOptFunction& func) { set_socket_opt_func_ = func; }

  /// @brief Get the function of set socket option after the server accepts the connection
  SetSocketOptFunction& GetAcceptSetSocketOptFunction() { return set_socket_opt_func_; }

 private:
  AcceptConnectionFunction accept_handler_{nullptr};

  SetSocketOptFunction set_socket_opt_func_{nullptr};
};

}  // namespace trpc
