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

#include "trpc/runtime/iomodel/reactor/default/tcp_acceptor.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc {

TcpAcceptor::TcpAcceptor(Reactor* reactor, const NetworkAddress& tcp_addr)
    : reactor_(reactor),
      tcp_addr_(tcp_addr),
      socket_(Socket::CreateTcpSocket(tcp_addr.IsIpv6())),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
}

TcpAcceptor::~TcpAcceptor() {
  socket_.Close();
  close(idle_fd_);

  if (GetInitializationState() != InitializationState::kInitializedSuccess) {
    return;
  }

  TRPC_ASSERT(!enable_);
}

bool TcpAcceptor::EnableListen(int backlog) {
  if (enable_) {
    return true;
  }

  if (!socket_.IsValid() || idle_fd_ < 0) {
    TRPC_LOG_ERROR("socket invalid.");
    return false;
  }

  SetFd(socket_.GetFd());

  socket_.SetReuseAddr();
  socket_.SetReusePort();
  socket_.SetTcpNoDelay();
  socket_.SetNoCloseWait();
  socket_.SetBlock(false);
  socket_.SetKeepAlive();

  auto& set_socket_opt_fun = GetAcceptSetSocketOptFunction();
  if (set_socket_opt_fun) {
    set_socket_opt_fun(socket_);
  }

  if (!socket_.Bind(tcp_addr_)) {
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

void TcpAcceptor::DisableListen() {
  if (!enable_) {
    return;
  }

  enable_ = false;

  DisableEvent(EventHandler::EventType::kReadEvent);

  reactor_->Update(this);

  Deref();
}

int TcpAcceptor::HandleReadEvent() {
  bool flag = true;
  while (flag) {
    NetworkAddress peer_addr;
    int conn_fd = socket_.Accept(&peer_addr);
    if (conn_fd >= 0) {
      AcceptConnectionFunction& accept_handler = GetAcceptHandleFunction();
      if (accept_handler) {
        AcceptConnectionInfo info;

        info.socket = Socket(conn_fd, socket_.GetDomain());
        info.conn_info.local_addr = tcp_addr_;
        info.conn_info.remote_addr = std::move(peer_addr);

        if (!accept_handler(info)) {
          // Do not close socket when accept_handler fail(accept_handler will close), or may double-close socket
          TRPC_LOG_ERROR("tcp accept handler return false");
        }
      } else {
        TRPC_ASSERT("tcp accept handler empty");
        close(conn_fd);
      }
    } else {
      if (errno == EMFILE) {
        close(idle_fd_);

        idle_fd_ = socket_.Accept(&peer_addr);

        close(idle_fd_);

        idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);

        TRPC_LOG_ERROR("tcp accept failed:too many open files");
      } else if (errno == EAGAIN) {
        flag = false;
      }
    }
  }

  return 0;
}

}  // namespace trpc
