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

#include <memory>

#include "trpc/runtime/iomodel/reactor/common/accept_connection_info.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/common/unix_address.h"
#include "trpc/runtime/iomodel/reactor/default/acceptor.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"

namespace trpc {

/// @brief The acceptor for uds
class UdsAcceptor final : public Acceptor {
 public:
  explicit UdsAcceptor(Reactor* reactor, const UnixAddress& unix_addr);

  ~UdsAcceptor() override;

  bool EnableListen(int backlog = 1024) override;

  void DisableListen() override;

 protected:
  int HandleReadEvent() override;

 private:
  Reactor* reactor_{nullptr};

  UnixAddress unix_addr_;

  Socket socket_;

  // idle fd, for EMFILE
  int idle_fd_{-1};

  bool enable_{false};
};

}  // namespace trpc
