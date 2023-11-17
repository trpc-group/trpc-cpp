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

#include "trpc/runtime/iomodel/reactor/default/uds_acceptor.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc {

UdsAcceptor::UdsAcceptor(Reactor* reactor, const UnixAddress& unix_addr)
    : reactor_(reactor),
      unix_addr_(unix_addr),
      socket_(Socket::CreateUnixSocket()),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
}

UdsAcceptor::~UdsAcceptor() {
  socket_.Close();
  if (idle_fd_ >= 0) {
    ::close(idle_fd_);
  }

  if (GetInitializationState() != InitializationState::kInitializedSuccess) {
    return;
  }

  TRPC_ASSERT(!enable_);
}

bool UdsAcceptor::EnableListen(int backlog) {
  if (enable_) {
    return true;
  }

  if (!socket_.IsValid() || idle_fd_ < 0) {
    TRPC_LOG_ERROR("socket invalid.");
    return false;
  }

  SetFd(socket_.GetFd());

  socket_.SetBlock(false);
  auto& set_socket_opt_fun = GetAcceptSetSocketOptFunction();
  if (set_socket_opt_fun) {
    set_socket_opt_fun(socket_);
  }

  if (!socket_.Bind(unix_addr_)) {
    return false;
  }

  if (!socket_.Listen(backlog)) {
    return false;
  }

  EnableEvent(EventHandler::EventType::kReadEvent);

  Ref();

  reactor_->Update(this);

  enable_ = true;

  SetInitializationState(InitializationState::kInitializedSuccess);

  return true;
}

void UdsAcceptor::DisableListen() {
  if (!enable_) {
    return;
  }

  enable_ = false;

  DisableEvent(EventHandler::EventType::kReadEvent);

  reactor_->Update(this);

  Deref();
}

int UdsAcceptor::HandleReadEvent() {
  bool flag = true;
  while (flag) {
    UnixAddress peer_addr;
    int conn_fd = socket_.Accept(&peer_addr);
    if (conn_fd >= 0) {
      AcceptConnectionFunction& accept_handler = GetAcceptHandleFunction();
      if (accept_handler) {
        AcceptConnectionInfo info;

        info.socket = Socket(conn_fd, socket_.GetDomain());
        info.conn_info.un_local_addr = unix_addr_;
        info.conn_info.un_remote_addr = std::move(peer_addr);
        info.conn_info.is_net = false;

        if (!accept_handler(info)) {
          // Do not close socket when accept_handler fail(accept_handler will close), or may double-close socket
          TRPC_LOG_ERROR("unix accept handler return false");
        }
      } else {
        TRPC_ASSERT("unix accept handler empty");
        close(conn_fd);
      }
    } else {
      if (errno == EMFILE) {
        close(idle_fd_);

        idle_fd_ = socket_.Accept(&peer_addr);

        close(idle_fd_);

        idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);

        TRPC_LOG_ERROR("unix accept failed:too many open files");
      } else if (errno == EAGAIN) {
        flag = false;
      }
    }
  }

  return 0;
}

}  // namespace trpc
