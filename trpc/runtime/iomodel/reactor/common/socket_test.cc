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

#include <ifaddrs.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>
#include <random>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/util/net_util.h"

namespace trpc {

namespace testing {

class MockSocket : public Socket {};

class SocketTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    ifaddrs *ifa = nullptr;
    getifaddrs(&ifa);

    for (ifaddrs *ifp = ifa; ifp != nullptr; ifp = ifp->ifa_next) {
      if (!ifp->ifa_addr) {
        continue;
      }
      if (ifa->ifa_addr->sa_family == AF_INET6) {
        ipv6_enable_ = true;
      }
    }
    if (ifa != nullptr) {
      freeifaddrs(ifa);
    }
  }

  void SetUp() override {
    tcp_ipv4_server_sock_ = std::make_unique<Socket>(Socket::CreateTcpSocket(false));
    tcp_ipv4_client_sock_ = std::make_unique<Socket>(Socket::CreateTcpSocket(false));

    udp_ipv4_server_sock_ = std::make_unique<Socket>(Socket::CreateUdpSocket(false));
    udp_ipv4_client_sock_ = std::make_unique<Socket>(Socket::CreateUdpSocket(false));

    unix_server_sock_ = std::make_unique<Socket>(Socket::CreateUnixSocket());
    unix_client_sock_ = std::make_unique<Socket>(Socket::CreateUnixSocket());

    if (ipv6_enable_) {
      tcp_ipv6_server_sock_ = std::make_unique<Socket>(Socket::CreateTcpSocket(true));
      tcp_ipv6_client_sock_ = std::make_unique<Socket>(Socket::CreateTcpSocket(true));

      udp_ipv6_server_sock_ = std::make_unique<Socket>(Socket::CreateUdpSocket(true));
      udp_ipv6_client_sock_ = std::make_unique<Socket>(Socket::CreateUdpSocket(true));
    }
  }

 protected:
  inline static bool ipv6_enable_ = false;

  std::unique_ptr<Socket> tcp_ipv4_server_sock_;
  std::unique_ptr<Socket> tcp_ipv4_client_sock_;

  std::unique_ptr<Socket> tcp_ipv6_server_sock_;
  std::unique_ptr<Socket> tcp_ipv6_client_sock_;

  std::unique_ptr<Socket> udp_ipv4_server_sock_;
  std::unique_ptr<Socket> udp_ipv4_client_sock_;

  std::unique_ptr<Socket> unix_server_sock_;
  std::unique_ptr<Socket> unix_client_sock_;

  std::unique_ptr<Socket> udp_ipv6_server_sock_;
  std::unique_ptr<Socket> udp_ipv6_client_sock_;
};

TEST_F(SocketTest, GetDomain) {
  EXPECT_EQ(AF_INET, tcp_ipv4_server_sock_->GetDomain());
  EXPECT_EQ(AF_INET, udp_ipv4_server_sock_->GetDomain());
  EXPECT_EQ(AF_UNIX, unix_server_sock_->GetDomain());

  if (ipv6_enable_) {
    EXPECT_EQ(AF_INET6, tcp_ipv6_server_sock_->GetDomain());
    EXPECT_EQ(AF_INET6, udp_ipv6_server_sock_->GetDomain());
  }
}

TEST_F(SocketTest, GetFd) {
  EXPECT_GE(tcp_ipv4_server_sock_->GetFd(), 0);
  EXPECT_GE(udp_ipv4_server_sock_->GetFd(), 0);
  EXPECT_GE(unix_server_sock_->GetFd(), 0);

  if (ipv6_enable_) {
    EXPECT_GE(tcp_ipv6_server_sock_->GetFd(), 0);
    EXPECT_GE(udp_ipv6_server_sock_->GetFd(), 0);
  }
}

TEST_F(SocketTest, IsValid) {
  EXPECT_TRUE(tcp_ipv4_server_sock_->IsValid());
  EXPECT_TRUE(udp_ipv4_server_sock_->IsValid());
  EXPECT_TRUE(unix_server_sock_->IsValid());

  if (ipv6_enable_) {
    EXPECT_TRUE(tcp_ipv6_server_sock_->IsValid());
    EXPECT_TRUE(udp_ipv6_server_sock_->IsValid());
  }
}

TEST_F(SocketTest, Close) {
  tcp_ipv4_server_sock_->Close();
  EXPECT_FALSE(tcp_ipv4_server_sock_->IsValid());
  EXPECT_EQ(-1, tcp_ipv4_server_sock_->GetFd());
}

TEST_F(SocketTest, ReuseAddr) {
  tcp_ipv4_server_sock_->SetReuseAddr();
  int opt = 0;
  socklen_t opt_len = static_cast<socklen_t>(sizeof(opt));
  tcp_ipv4_server_sock_->GetSockOpt(SO_REUSEADDR, &opt, &opt_len, SOL_SOCKET);
  EXPECT_EQ(1, opt);
}

TEST_F(SocketTest, ReusePort) {
#if defined(SO_REUSEPORT) && !defined(TRPC_DISABLE_REUSEPORT)
  tcp_ipv4_server_sock_->SetReusePort();
  int opt = 0;
  socklen_t opt_len = static_cast<socklen_t>(sizeof(opt_len));
  tcp_ipv4_server_sock_->GetSockOpt(SO_REUSEPORT, &opt, &opt_len, SOL_SOCKET);
  EXPECT_EQ(1, opt);
#endif
}

TEST_F(SocketTest, SetBlock) {
  tcp_ipv4_server_sock_->SetBlock(true);
  tcp_ipv4_server_sock_->SetBlock(false);

  unix_server_sock_->SetBlock(true);
  unix_server_sock_->SetBlock(false);
}

TEST_F(SocketTest, CloseWait) {
  tcp_ipv4_server_sock_->SetNoCloseWait();
  tcp_ipv4_server_sock_->SetCloseWait(30);
  tcp_ipv4_server_sock_->SetCloseWaitDefault();
}

TEST_F(SocketTest, TcpNoDelay) {
  tcp_ipv4_server_sock_->SetTcpNoDelay();
  int opt = 0;
  socklen_t opt_len = static_cast<socklen_t>(sizeof(opt));
  tcp_ipv4_server_sock_->GetSockOpt(TCP_NODELAY, &opt, &opt_len, IPPROTO_TCP);
  EXPECT_EQ(1, opt);
}

TEST_F(SocketTest, KeepAlive) {
  tcp_ipv4_client_sock_->SetKeepAlive();
  int opt = 0;
  socklen_t opt_len = static_cast<socklen_t>(sizeof(opt));
  tcp_ipv4_client_sock_->GetSockOpt(SO_KEEPALIVE, &opt, &opt_len, SOL_SOCKET);
  EXPECT_EQ(1, opt);
}

TEST_F(SocketTest, SendBufferSize) {
  // The kernel will double this value. see `man 7 socket`
  tcp_ipv4_server_sock_->SetSendBufferSize(10240);
  EXPECT_EQ(2 * 10240, tcp_ipv4_server_sock_->GetSendBufferSize());
}

TEST_F(SocketTest, RecvBuffeSize) {
  // The kernel will double this value. see `man 7 socket`
  tcp_ipv4_server_sock_->SetRecvBufferSize(10240);
  EXPECT_EQ(2 * 10240, tcp_ipv4_server_sock_->GetRecvBufferSize());
}

TEST_F(SocketTest, TcpCommunicattion) {
  char message[] = "helloworld";

  {
    NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV4);
    tcp_ipv4_server_sock_->SetReuseAddr();
    tcp_ipv4_server_sock_->Bind(addr);
    tcp_ipv4_server_sock_->Listen();

    std::thread t1([&]() {
      sleep(1);
      NetworkAddress addr;
      int conn_fd = tcp_ipv4_server_sock_->Accept(&addr);
      Socket sock(conn_fd, AF_INET);

      char recvbuf[1024] = {0};
      sock.Recv(recvbuf, sizeof(recvbuf));

      EXPECT_STREQ(message, recvbuf);
    });

    tcp_ipv4_client_sock_->Connect(addr);
    tcp_ipv4_client_sock_->Send(message, sizeof(message));

    t1.join();
  }

  if (ipv6_enable_) {
    NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV6);
    tcp_ipv4_server_sock_->SetReuseAddr();
    tcp_ipv6_server_sock_->Bind(addr);
    tcp_ipv6_server_sock_->Listen();

    std::thread t1([&]() {
      sleep(1);
      NetworkAddress addr;
      int conn_fd = tcp_ipv6_server_sock_->Accept(&addr);
      Socket sock(conn_fd, AF_INET6);

      char recvbuf[1024] = {0};
      sock.Recv(recvbuf, sizeof(recvbuf), 0);

      EXPECT_STREQ(message, recvbuf);
    });

    tcp_ipv6_client_sock_->Connect(addr);
    tcp_ipv6_client_sock_->Send(message, sizeof(message));
  }
}

TEST_F(SocketTest, UdpCommunicattion) {
  char message[] = "helloworld";
  {
    NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV4);
    udp_ipv4_server_sock_->SetReuseAddr();
    udp_ipv4_server_sock_->Bind(addr);

    std::thread t1([&]() {
      char recvbuf[1024] = {0};
      udp_ipv4_server_sock_->RecvFrom(recvbuf, sizeof(recvbuf), 0, nullptr);

      EXPECT_STREQ(message, recvbuf);
    });
    sleep(1);
    udp_ipv4_client_sock_->SendTo(message, sizeof(message), 0, addr);
    t1.join();
  }

  if (ipv6_enable_) {
    NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV6);
    tcp_ipv4_server_sock_->SetReuseAddr();
    udp_ipv4_server_sock_->Bind(addr);
    udp_ipv4_server_sock_->Listen();

    std::thread t2([&]() {
      char recvbuf[1024] = {0};
      udp_ipv6_server_sock_->RecvFrom(recvbuf, sizeof(recvbuf), 0, nullptr);

      EXPECT_STREQ(message, recvbuf);
    });

    sleep(1);
    udp_ipv6_client_sock_->SendTo(message, sizeof(message), 0, addr);
    t2.join();
  }
}

TEST_F(SocketTest, UnixCommunicattion) {
  char message[] = "helloworld";

  {
    char path[] = "trpc_unix_test.socket";
    UnixAddress addr(path);
    unix_server_sock_->Bind(addr);
    unix_server_sock_->Listen();

    std::thread t1([&]() {
      sleep(1);
      UnixAddress addr;
      int conn_fd = unix_server_sock_->Accept(&addr);
      Socket sock(conn_fd, AF_UNIX);

      char recvbuf[1024] = {0};
      sock.Recv(recvbuf, sizeof(recvbuf));

      EXPECT_STREQ(message, recvbuf);
    });

    sleep(1);
    unix_client_sock_->Connect(addr);
    unix_client_sock_->Send(message, sizeof(message));

    t1.join();
  }
}

TEST_F(SocketTest, BadTcpSocket) {
  Socket bad_tcp_sock;

  EXPECT_FALSE(bad_tcp_sock.IsValid());

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetBlock(true);
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetNoCloseWait();
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetCloseWait(10);
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetCloseWaitDefault();
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetTcpNoDelay();
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetKeepAlive();
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetSendBufferSize(1024);
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.GetSendBufferSize();
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.SetRecvBufferSize(1024);
  EXPECT_EQ(9, errno);

  // errno: 9, Bad file descriptor
  errno = 0;
  bad_tcp_sock.GetRecvBufferSize();
  EXPECT_EQ(9, errno);
}

TEST_F(SocketTest, BindFail) {
  // Invalid IP 1.1.1.1，socket bind will fail
  NetworkAddress bad_network_addr = NetworkAddress("1.1.1.1", 10000, NetworkAddress::IpType::kIpV4);
  bool network_bind_fail = tcp_ipv4_server_sock_->Bind(bad_network_addr);
  EXPECT_EQ(network_bind_fail, false);

  sockaddr_un bad_unix_socket;
  UnixAddress bad_unix_addr = UnixAddress(&bad_unix_socket);
  bool unix_bind_fail = unix_server_sock_->Bind(bad_unix_addr);
  EXPECT_EQ(unix_bind_fail, false);
}

}  // namespace testing

}  // namespace trpc
