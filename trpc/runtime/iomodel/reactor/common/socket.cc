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

#include "trpc/runtime/iomodel/reactor/common/socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

#include "trpc/util/log/logging.h"

namespace trpc {

Socket Socket::CreateTcpSocket(bool ipv6) {
  int domain = ipv6 ? AF_INET6 : AF_INET;
  int fd = ::socket(domain, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
  if (fd < 0) {
    TRPC_LOG_ERROR("create tcp socket failed, errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return Socket(fd, 0);
  }
  return Socket(fd, domain);
}

Socket Socket::CreateUdpSocket(bool ipv6) {
  int domain = ipv6 ? AF_INET6 : AF_INET;
  int fd = ::socket(domain, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) {
    TRPC_LOG_ERROR("create udp socket failed, errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return Socket(fd, 0);
  }
  return Socket(fd, domain);
}

Socket Socket::CreateUnixSocket() {
  int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    TRPC_LOG_ERROR("create unix socket failed, errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return Socket(fd, 0);
  }
  return Socket(fd, AF_UNIX);
}

int Socket::Accept(NetworkAddress* peer_addr) {
  int fd = -1;
  struct sockaddr_storage addr_stroage;
  struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&addr_stroage);
  socklen_t len = static_cast<socklen_t>(sizeof(addr_stroage));
  while ((fd = ::accept4(fd_, addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 && errno == EINTR) {
  }

  if (fd >= 0 && peer_addr != nullptr) {
    *peer_addr = NetworkAddress(addr);
  }

  return fd;
}

int Socket::Accept(UnixAddress* peer_addr) {
  int fd = -1;
  struct sockaddr_un addr;
  socklen_t len = static_cast<socklen_t>(sizeof(addr));
  while ((fd = ::accept4(fd_, reinterpret_cast<struct sockaddr*>(&addr), &len,
                         SOCK_NONBLOCK | SOCK_CLOEXEC)) < 0 &&
         errno == EINTR) {
  }

  if (peer_addr != nullptr) {
    *peer_addr = UnixAddress(&addr);
  }

  return fd;
}

bool Socket::SetReuseAddr() {
  int flag = 1;
  if (SetSockOpt(SO_REUSEADDR, static_cast<const void*>(&flag),
                 static_cast<socklen_t>(sizeof(flag)), SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("SetReuseAddr failed" << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }

  return true;
}

bool Socket::SetReusePort() {
#if defined(SO_REUSEPORT) && !defined(TRPC_DISABLE_REUSEPORT)
  int flag = 1;
  if (SetSockOpt(SO_REUSEPORT, static_cast<const void*>(&flag),
                 static_cast<socklen_t>(sizeof(flag)), SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("SetReusePort failed" << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
#endif
  return true;
}

bool Socket::Bind(const NetworkAddress& bind_addr) {
  int ret = ::bind(fd_, bind_addr.SockAddr(), bind_addr.Socklen());
  if (ret != 0) {
    TRPC_FMT_CRITICAL("Bind address {} error: {}", bind_addr.ToString(), strerror(errno));
    return false;
  }

  return true;
}

bool Socket::Bind(const UnixAddress& bind_addr) {
  if (::access(bind_addr.Path(), 0) != -1) {
    ::remove(bind_addr.Path());
  }
  int ret = ::bind(fd_, bind_addr.SockAddr(), bind_addr.Socklen());
  if (ret != 0) {
    TRPC_FMT_CRITICAL("Bind address {} error: {}", bind_addr.Path(), strerror(errno));
    return false;
  }

  return true;
}

void Socket::Close() {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool Socket::Listen(int backlog) {
  if (::listen(fd_, backlog) < 0) {
    TRPC_LOG_ERROR("Listen failed" << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
  return true;
}

int Socket::Connect(const NetworkAddress& addr) {
  if (::connect(fd_, addr.SockAddr(), addr.Socklen()) < 0 && errno != EINPROGRESS) {
    return -1;
  }
  return 0;
}

int Socket::Connect(const UnixAddress& addr) {
  if (::connect(fd_, addr.SockAddr(), addr.Socklen()) < 0 && errno != EINPROGRESS) {
    return -1;
  }
  return 0;
}

int Socket::Recv(void* buff, size_t len, int flag) { return ::recv(fd_, buff, len, flag); }

int Socket::Send(const void* buff, size_t len, int flag) { return ::send(fd_, buff, len, flag); }

int Socket::RecvFrom(void* buff, size_t len, int flag, NetworkAddress* peer_addr) {
  struct sockaddr addr;
  socklen_t sock_len = static_cast<socklen_t>(sizeof(addr));
  int ret = ::recvfrom(fd_, buff, len, flag, &addr, &sock_len);
  if (ret > 0 && peer_addr != nullptr) {
    *peer_addr = NetworkAddress(&addr);
  }
  return ret;
}

int Socket::SendTo(const void* buff, size_t len, int flag, const NetworkAddress& peer_addr) {
  return ::sendto(fd_, buff, len, flag, peer_addr.SockAddr(), peer_addr.Socklen());
}

int Socket::Writev(const struct iovec* iov, int iovcnt) { return ::writev(fd_, iov, iovcnt); }

int Socket::SendMsg(const struct msghdr* msg, int flag) { return ::sendmsg(fd_, msg, flag); }

int Socket::RecvMsg(msghdr* message, int flag, NetworkAddress* peer_addr) {
  struct sockaddr addr;
  socklen_t sock_len = static_cast<socklen_t>(sizeof(addr));
  message->msg_name = &addr;
  message->msg_namelen = sock_len;
  int ret = ::recvmsg(fd_, message, flag);
  if (ret > 0 && peer_addr != nullptr) {
    *peer_addr = NetworkAddress(&addr);
  }
  return ret;
}

bool Socket::SetBlock(bool block) {
  int val = 0;

  if ((val = ::fcntl(fd_, F_GETFL, 0)) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }

  if (!block) {
    val |= O_NONBLOCK;
  } else {
    val &= ~O_NONBLOCK;
  }

  if (::fcntl(fd_, F_SETFL, val) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
  return true;
}

int Socket::SetSockOpt(int opt, const void* val, socklen_t opt_len, int level) {
  return ::setsockopt(fd_, level, opt, val, opt_len);
}

int Socket::GetSockOpt(int opt, void* val, socklen_t* opt_len, int level) {
  return ::getsockopt(fd_, level, opt, val, opt_len);
}

bool Socket::SetNoCloseWait() {
  struct linger ling;
  ling.l_onoff = 1;
  ling.l_linger = 0;

  if (SetSockOpt(SO_LINGER, static_cast<const void*>(&ling), static_cast<socklen_t>(sizeof(linger)),
                 SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
  return true;
}

bool Socket::SetCloseWait(int delay) {
  struct linger ling;
  ling.l_onoff = 1;
  ling.l_linger = delay;

  if (SetSockOpt(SO_LINGER, static_cast<const void*>(&ling), static_cast<socklen_t>(sizeof(linger)),
                 SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
  return true;
}

bool Socket::SetCloseWaitDefault() {
  struct linger ling;
  ling.l_onoff = 0;
  ling.l_linger = 0;

  if (SetSockOpt(SO_LINGER, static_cast<const void*>(&ling), static_cast<socklen_t>(sizeof(linger)),
                 SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
  return true;
}

bool Socket::SetTcpNoDelay() {
  int flag = 1;
  if (SetSockOpt(TCP_NODELAY, static_cast<const void*>(&flag), static_cast<socklen_t>(sizeof(flag)),
                 IPPROTO_TCP) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }

#ifdef TRPC_DISABLE_TCP_CORK
  // After Linux2.6, TCP_CORK can be mixed with TCP_NODELAY,
  // and the priority of TCP_CORK is higher than TCP_NODELAY.
  // therefore, it must be explicitly disabled to avoid
  // the problem of delayed sending of small packets.
  int cork_state = 0;
  if (SetSockOpt(TCP_CORK, static_cast<const void*>(&cork_state),
                 static_cast<socklen_t>(sizeof(cork_state)), IPPROTO_TCP) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
#endif
  return true;
}

bool Socket::SetKeepAlive() {
  int flag = 1;
  if (SetSockOpt(SO_KEEPALIVE, static_cast<const void*>(&flag),
                 static_cast<socklen_t>(sizeof(flag)), SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
    return false;
  }
  return true;
}

void Socket::SetSendBufferSize(int sz) {
  int flag = 1;
  if (SetSockOpt(SO_SNDBUF, static_cast<const void*>(&sz), static_cast<socklen_t>(sizeof(flag)),
                 SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
  }
}

int Socket::GetSendBufferSize() {
  int sz = 0;
  socklen_t len = sizeof(sz);
  if (GetSockOpt(SO_SNDBUF, static_cast<void*>(&sz), &len, SOL_SOCKET) == -1 || len != sizeof(sz)) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
  }

  return sz;
}

void Socket::SetRecvBufferSize(int sz) {
  if (SetSockOpt(SO_RCVBUF, static_cast<const void*>(&sz), static_cast<socklen_t>(sizeof(sz)),
                 SOL_SOCKET) == -1) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
  }
}

int Socket::GetRecvBufferSize() {
  int sz = 0;
  socklen_t len = sizeof(sz);
  if (GetSockOpt(SO_RCVBUF, static_cast<void*>(&sz), &len, SOL_SOCKET) == -1 || len != sizeof(sz)) {
    TRPC_LOG_ERROR("setsockopt failed, fd: " << fd_ << ", errno: " << errno <<
                   ", error msg: " << strerror(errno));
  }

  return sz;
}

}  // namespace trpc
