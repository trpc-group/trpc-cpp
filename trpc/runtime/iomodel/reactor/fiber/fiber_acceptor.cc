//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/native/acceptor.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/fiber_acceptor.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc {

FiberAcceptor::FiberAcceptor(Reactor* reactor, const NetworkAddress& tcp_addr)
    : FiberConnection(reactor),
      socket_(Socket::CreateTcpSocket(tcp_addr.IsIpv6())),
      is_net_(true),
      tcp_addr_(tcp_addr),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
}

FiberAcceptor::FiberAcceptor(Reactor* reactor, const UnixAddress& unix_addr)
    : FiberConnection(reactor),
      socket_(Socket::CreateUnixSocket()),
      is_net_(false),
      unix_addr_(unix_addr),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
}

FiberAcceptor::~FiberAcceptor() {
  socket_.Close();

  if (idle_fd_ >= 0) {
    ::close(idle_fd_);
  }

  if (GetInitializationState() != InitializationState::kInitializedSuccess) {
    return;
  }

  TRPC_ASSERT(!socket_.IsValid());
}

bool FiberAcceptor::Listen() {
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
  if (set_socket_opt_func_) {
    set_socket_opt_func_(socket_);
  }

  if (is_net_) {
    if (!socket_.Bind(tcp_addr_)) {
      return false;
    }
  } else {
    if (!socket_.Bind(unix_addr_)) {
      return false;
    }
  }

  if (!socket_.Listen(1024)) {
    return false;
  }

  EnableEvent(EventHandler::EventType::kReadEvent);

  AttachReactor();

  SetInitializationState(InitializationState::kInitializedSuccess);

  return true;
}

void FiberAcceptor::Stop() {
  if (GetInitializationState() == InitializationState::kInitializedSuccess) {
    Kill(CleanupReason::UserInitiated);
  }
}

void FiberAcceptor::Join() {
  if (GetInitializationState() == InitializationState::kInitializedSuccess) {
    WaitForCleanup();
  }
}

FiberConnection::EventAction FiberAcceptor::OnReadable() {
  if (is_net_) {
    return OnTcpReadable();
  } else {
    return OnUdsReadable();
  }
}

FiberConnection::EventAction FiberAcceptor::OnTcpReadable() {
  while (true) {
    NetworkAddress peer_addr;
    int conn_fd = socket_.Accept(&peer_addr);
    if (conn_fd >= 0) {
      if (accept_handler_) {
        AcceptConnectionInfo info;

        info.socket = Socket(conn_fd, socket_.GetDomain());
        info.conn_info.local_addr = tcp_addr_;
        info.conn_info.remote_addr = std::move(peer_addr);

        if (!accept_handler_(info)) {
          // Do not close socket when accept_handler fail(accept_handler will close), or may double-close socket
          TRPC_LOG_ERROR("FiberAcceptor::OnTcpReadable accept handler return false");
        }
      } else {
        TRPC_LOG_ERROR("FiberAcceptor::OnTcpReadable accept handler empty.");
        ::close(conn_fd);
      }
    } else {
      if (errno == EMFILE) {
        ::close(idle_fd_);

        idle_fd_ = socket_.Accept(&peer_addr);

        ::close(idle_fd_);

        idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);

        TRPC_LOG_ERROR("FiberAcceptor::OnTcpReadable tcp accept too many open files");
      } else if (errno == EAGAIN) {
        return EventAction::kReady;
      } else {
        TRPC_LOG_ERROR("FiberAcceptor::OnTcpReadable errno:" << errno << ", msg:" << strerror(errno));
      }
    }
  }
}

FiberConnection::EventAction FiberAcceptor::OnUdsReadable() {
  while (true) {
    UnixAddress peer_addr;
    int conn_fd = socket_.Accept(&peer_addr);
    if (conn_fd >= 0) {
      if (accept_handler_) {
        AcceptConnectionInfo info;

        info.socket = Socket(conn_fd, socket_.GetDomain());
        info.conn_info.un_local_addr = unix_addr_;
        info.conn_info.un_remote_addr = std::move(peer_addr);
        info.conn_info.is_net = false;

        if (!accept_handler_(info)) {
          // Do not close socket when accept_handler fail(accept_handler will close), or may double-close socket
          TRPC_LOG_ERROR("FiberAcceptor::OnUdsReadable accept handler return false");
        }
      } else {
        TRPC_LOG_ERROR("FiberAcceptor::OnUdsReadable accept handler empty.");
        ::close(conn_fd);
      }
    } else {
      if (errno == EMFILE) {
        ::close(idle_fd_);

        idle_fd_ = socket_.Accept(&peer_addr);

        ::close(idle_fd_);

        idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);

        TRPC_LOG_ERROR("FiberAcceptor::OnUdsReadable tcp accept too many open files");
      } else if (errno == EAGAIN) {
        return EventAction::kReady;
      } else {
        TRPC_LOG_ERROR("FiberAcceptor::OnUdsReadable errno:" << errno << ", msg:" << strerror(errno));
      }
    }
  }
}

FiberConnection::EventAction FiberAcceptor::OnWritable() {
  if (GetInitializationState() == InitializationState::kInitializedSuccess) {
    TRPC_ASSERT(false);
  }
  return FiberConnection::EventAction::kReady;
}

void FiberAcceptor::OnError(int err) {
  if (GetInitializationState() == InitializationState::kInitializedSuccess) {
    TRPC_ASSERT(false);
  }
}

void FiberAcceptor::OnCleanup(CleanupReason reason) { socket_.Close(); }

}  // namespace trpc
