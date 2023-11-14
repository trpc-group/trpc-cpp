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

#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <functional>

#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/common/unix_address.h"

namespace trpc {

/// @brief Socket operation wrapper class
class Socket {
 public:
  Socket() : fd_(-1), domain_(0) {}

  explicit Socket(int fd, int domain) : fd_(fd), domain_(domain) {}

  /// @brief Create tcp socket
  static Socket CreateTcpSocket(bool ipv6 = false);

  /// @brief Create udp socket
  static Socket CreateUdpSocket(bool ipv6 = false);

  /// @brief Create uds socket
  static Socket CreateUnixSocket();

  /// @brief Get socket domain
  int GetDomain() const { return domain_; }

  /// @brief Get socket fd
  int GetFd() const { return fd_; }

  /// @brief Check socket whether to be valid
  bool IsValid() const { return fd_ >= 0; }

  /// @brief Close socket
  void Close();

  /// @brief Set socket option
  int SetSockOpt(int opt, const void* val, socklen_t opt_len, int level = SOL_SOCKET);

  /// @brief Get socket option
  int GetSockOpt(int opt, void* val, socklen_t* opt_len, int level = SOL_SOCKET);

  /// @brief Accecpt tcp connection
  int Accept(NetworkAddress* peer_addr);

  /// @brief Accecpt unix connection
  int Accept(UnixAddress* peer_addr);

  /// @brief Tcp bind
  bool Bind(const NetworkAddress& bind_addr);

  /// @brief Unix bind
  bool Bind(const UnixAddress& bind_addr);

  /// @brief Listen tcp conneciton
  bool Listen(int backlog = SOMAXCONN);

  /// @brief Establish tcp connection
  int Connect(const NetworkAddress& addr);

  /// @brief Establish unix connection
  int Connect(const UnixAddress& addr);

  /// @brief Recv data
  int Recv(void* buff, size_t len, int flag = 0);

  /// @brief Send data
  int Send(const void* buff, size_t len, int flag = 0);

  /// @brief Recv udp data
  int RecvFrom(void* buff, size_t len, int flag, NetworkAddress* peer_addr);

  /// @brief Send udp data
  int SendTo(const void* buff, size_t len, int flag, const NetworkAddress& peer_addr);

  /// @brief Send tcp data
  int Writev(const struct iovec* iov, int iovcnt);

  /// @brief Send msg
  int SendMsg(const struct msghdr* msg, int flag = 0);

  /// @brief Recv msg
  int RecvMsg(msghdr* message, int flag, NetworkAddress* peer_addr);

  /// @brief Set SO_REUSEADD
  bool SetReuseAddr();

  /// @brief Set SO_REUSEPORT
  bool SetReusePort();

  /// @brief Set socket whether to be block
  bool SetBlock(bool block = false);

  /// @brief Set the option that discard all data in the send buffer when socket close
  bool SetNoCloseWait();

  /// @brief Set the delay time to wait for the buffer to continue sending when socket close
  bool SetCloseWait(int delay = 30);

  /// @brief Set system default behavior when socket clost
  bool SetCloseWaitDefault();

  /// @brief Set TCP_NODELAY
  bool SetTcpNoDelay();

  /// @brief Set SO_KEEPALIVE
  bool SetKeepAlive();

  /// @brief Get receive buffer size
  int GetRecvBufferSize();

  /// @brief Set receive buffer size
  void SetRecvBufferSize(int sz);

  /// @brief Get send buffer size
  int GetSendBufferSize();

  /// @brief Set end buffer size
  void SetSendBufferSize(int sz);

 private:
  int fd_;

  int domain_;
};

/// @brief The function for set socket option
using SetSocketOptFunction = std::function<void(Socket&)>;

}  // namespace trpc
