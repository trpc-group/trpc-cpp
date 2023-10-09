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

#include <functional>
#include <memory>
#include <utility>

#include "trpc/runtime/iomodel/reactor/common/accept_connection_info.h"
#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/common/unix_address.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_connection.h"

namespace trpc {

/// @brief The acceptor for fiber tcp/uds
class FiberAcceptor : public FiberConnection {
 public:
  explicit FiberAcceptor(Reactor* reactor, const NetworkAddress& tcp_addr);

  explicit FiberAcceptor(Reactor* reactor, const UnixAddress& unix_addr);

  ~FiberAcceptor() override;

  /// @brief Begin to listen connection
  bool Listen();

  /// @brief Stop listen
  void Stop();

  /// @brief Wait for the operation of FiberAcceptor running completed
  void Join();

  /// @brief Set the function of proccessing connection after the server accepts
  void SetAcceptHandleFunction(AcceptConnectionFunction&& func) { accept_handler_ = std::move(func); }

  /// @brief Set the function of set socket option after the server accepts the connection
  void SetAcceptSetSocketOptFunction(const SetSocketOptFunction& func) { set_socket_opt_func_ = func; }

 private:
  EventAction OnReadable() override;
  EventAction OnWritable() override;
  void OnError(int err) override;
  void OnCleanup(CleanupReason reason) override;
  EventAction OnTcpReadable();
  EventAction OnUdsReadable();

 private:
  Reactor* reactor_{nullptr};

  Socket socket_;

  // tcp or uds
  bool is_net_;

  // tcp listen addr
  NetworkAddress tcp_addr_;

  // uds listen addr
  UnixAddress unix_addr_;

  // the function of proccessing connection after the server accepts
  AcceptConnectionFunction accept_handler_;

  // the function of set socket option after the server accepts the connection
  SetSocketOptFunction set_socket_opt_func_{nullptr};

  // idle fd, for EMFILE error
  int idle_fd_;
};

}  // namespace trpc
