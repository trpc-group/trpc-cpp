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

#include "trpc/transport/server/fiber/fiber_server_transport_impl.h"

#include <chrono>
#include <deque>
#include <iostream>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/util/net_util.h"
#include "trpc/util/thread/latch.h"

using namespace std::literals;

namespace trpc::testing {

TEST(FiberServerTransportImplTest, Tcp) {
  FiberServerTransportImpl server_transport;

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    return PacketChecker::PACKET_FULL;
  };
  info.msg_handle_function = [&server_transport](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      context->SetNetType(ServerContext::NetType::kTcp);
      context->SetReserved(conn.Get());
      context->SetIp(conn->GetPeerIp());
      context->SetPort(conn->GetPeerPort());

      rsp->context = context;
      rsp->buffer = std::move(buff);

      server_transport.SendMsg(rsp);

      ++it;
    }

    return true;
  };
  info.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };

  server_transport.Bind(info);
  server_transport.Listen();

  NetworkAddress addr(info.ip, info.port, info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret = socket.Connect(addr);
  ASSERT_TRUE(ret == 0);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket.Send(message, sizeof(message));
  ASSERT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  socket.Recv(recvbuf, sizeof(recvbuf));

  ASSERT_STREQ(recvbuf, message);

  server_transport.Stop();
  server_transport.Destroy();
}

TEST(FiberServerTransportImplTest, TcpByAsyncSend) {
  FiberServerTransportImpl server_transport;

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  info.dispatch_accept_function = [](const AcceptConnectionInfo& info, std::size_t total_io_num) { return 0; };
  info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    return PacketChecker::PACKET_FULL;
  };
  info.msg_handle_function = [&server_transport](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      context->SetNetType(ServerContext::NetType::kTcp);
      context->SetIp(conn->GetPeerIp());
      context->SetPort(conn->GetPeerPort());

      rsp->context = context;
      rsp->buffer = std::move(buff);

      server_transport.SendMsg(rsp);

      ++it;
    }

    return true;
  };
  info.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };
  info.custom_set_accept_socket_opt_function = [](Socket& socket) {};

  server_transport.Bind(info);
  server_transport.Listen();

  NetworkAddress addr(info.ip, info.port, info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret = socket.Connect(addr);
  ASSERT_TRUE(ret == 0);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket.Send(message, sizeof(message));
  ASSERT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  socket.Recv(recvbuf, sizeof(recvbuf));

  ASSERT_STREQ(recvbuf, message);

  server_transport.Stop();
  server_transport.Destroy();
}

TEST(FiberServerTransportImplTest, Udp) {
  FiberServerTransportImpl server_transport;

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "udp";
  info.protocol = "raw";
  info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    return PacketChecker::PACKET_FULL;
  };
  info.msg_handle_function = [&](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      context->SetNetType(ServerContext::NetType::kUdp);
      context->SetReserved(conn.Get());
      context->SetIp(conn->GetPeerIp());
      context->SetPort(conn->GetPeerPort());

      rsp->context = context;
      rsp->buffer = std::move(buff);

      server_transport.SendMsg(rsp);

      ++it;
    }

    return true;
  };
  info.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };

  server_transport.Bind(info);
  server_transport.Listen();

  NetworkAddress addr(info.ip, info.port, info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket socket = Socket::CreateUdpSocket(addr.IsIpv6());

  char message[] = "helloworld, this is udp client";
  int send_size = socket.SendTo(message, sizeof(message), 0, addr);
  ASSERT_TRUE(send_size > 0);

  NetworkAddress peer_addr;
  char recvbuf[1024] = {0};
  socket.RecvFrom(recvbuf, sizeof(recvbuf), 0, &peer_addr);

  ASSERT_STREQ(recvbuf, message);

  server_transport.Stop();
  server_transport.Destroy();
}

TEST(FiberServerTransportImplTest, TcpUdp) {
  FiberServerTransportImpl server_transport;

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp,udp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    return PacketChecker::PACKET_FULL;
  };
  info.msg_handle_function = [&server_transport](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      if (conn->GetConnType() == ConnectionType::kTcpLong) {
        context->SetNetType(ServerContext::NetType::kTcp);
      } else if (conn->GetConnType() == ConnectionType::kUdp) {
        context->SetNetType(ServerContext::NetType::kUdp);
      } else {
        assert(false);
      }
      context->SetReserved(conn.Get());
      context->SetIp(conn->GetPeerIp());
      context->SetPort(conn->GetPeerPort());

      rsp->context = context;
      rsp->buffer = std::move(buff);

      server_transport.SendMsg(rsp);

      ++it;
    }

    return true;
  };
  info.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };

  server_transport.Bind(info);
  server_transport.Listen();

  NetworkAddress tcp_addr(info.ip, info.port,
                          info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket tcp_socket = Socket::CreateTcpSocket(tcp_addr.IsIpv6());

  int ret = tcp_socket.Connect(tcp_addr);
  ASSERT_TRUE(ret == 0);

  char tcp_message[] = "helloworld, this is tcp client";
  int tcp_send_size = tcp_socket.Send(tcp_message, sizeof(tcp_message));
  ASSERT_TRUE(tcp_send_size > 0);

  char tcp_recvbuf[1024] = {0};
  tcp_socket.Recv(tcp_recvbuf, sizeof(tcp_recvbuf));

  ASSERT_STREQ(tcp_recvbuf, tcp_message);

  NetworkAddress udp_addr(info.ip, info.port,
                          info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket udp_socket = Socket::CreateUdpSocket(udp_addr.IsIpv6());

  char udp_message[] = "helloworld, this is udp client";
  int udp_send_size = udp_socket.SendTo(udp_message, sizeof(udp_message), 0, udp_addr);
  ASSERT_TRUE(udp_send_size > 0);

  NetworkAddress peer_addr;
  char udp_recvbuf[1024] = {0};
  udp_socket.RecvFrom(udp_recvbuf, sizeof(udp_recvbuf), 0, &peer_addr);

  ASSERT_STREQ(udp_recvbuf, udp_message);

  server_transport.Stop();
  server_transport.Destroy();
}

TEST(FiberServerTransportImplTest, StopListen) {
  FiberServerTransportImpl server_transport;

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    return PacketChecker::PACKET_FULL;
  };
  info.msg_handle_function = [&server_transport](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      context->SetNetType(ServerContext::NetType::kTcp);
      context->SetReserved(conn.Get());
      context->SetIp(conn->GetPeerIp());
      context->SetPort(conn->GetPeerPort());

      rsp->context = context;
      rsp->buffer = std::move(buff);

      server_transport.SendMsg(rsp);

      ++it;
    }

    return true;
  };
  info.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };

  server_transport.Bind(info);
  server_transport.Listen();

  NetworkAddress addr(info.ip, info.port, info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket socket1 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret1 = socket1.Connect(addr);
  ASSERT_TRUE(ret1 == 0);

  server_transport.StopListen(false);

  FiberSleepFor(std::chrono::milliseconds(10));

  Socket socket2 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret2 = socket2.Connect(addr);
  ASSERT_TRUE(ret2 == -1);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket1.Send(message, sizeof(message));
  ASSERT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  socket1.Recv(recvbuf, sizeof(recvbuf));

  ASSERT_STREQ(recvbuf, message);

  socket1.Close();
  socket2.Close();

  server_transport.Stop();
  server_transport.Destroy();
}

TEST(FiberServerTransportImplTest, StopListenWithClean) {
  FiberServerTransportImpl server_transport;

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    return PacketChecker::PACKET_FULL;
  };
  info.msg_handle_function = [&server_transport](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      context->SetNetType(ServerContext::NetType::kTcp);
      context->SetReserved(conn.Get());
      context->SetIp(conn->GetPeerIp());
      context->SetPort(conn->GetPeerPort());

      rsp->context = context;
      rsp->buffer = std::move(buff);

      server_transport.SendMsg(rsp);

      ++it;
    }

    return true;
  };
  info.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };

  server_transport.Bind(info);
  server_transport.Listen();

  NetworkAddress addr(info.ip, info.port, info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket socket1 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret1 = socket1.Connect(addr);
  ASSERT_TRUE(ret1 == 0);

  struct timeval timeout = {3, 0};
  ret1 = socket1.SetSockOpt(SO_RCVTIMEO, static_cast<const void*>(&timeout),
                  static_cast<socklen_t>(sizeof(timeout)), SOL_SOCKET);
  EXPECT_TRUE(ret1 == 0);

  server_transport.StopListen(true);

  FiberSleepFor(std::chrono::milliseconds(10));

  Socket socket2 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret2 = socket2.Connect(addr);
  ASSERT_TRUE(ret2 == -1);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket1.Send(message, sizeof(message));
  ASSERT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  int recv_size = socket1.Recv(recvbuf, sizeof(recvbuf));
  ASSERT_TRUE(recv_size < 0);

  socket1.Close();
  socket2.Close();

  server_transport.Stop();
  server_transport.Destroy();
}

TEST(FiberServerTransportImplTest, TcpFail) {
  FiberServerTransportImpl server_transport;

  trpc::BindInfo bad_bind_info;
  bad_bind_info.socket_type = "net";
  // invalid ip 1.1.1.1
  bad_bind_info.ip = "1.1.1.1";
  bad_bind_info.is_ipv6 = false;
  bad_bind_info.port = 11111;
  bad_bind_info.network = "tcp";
  bad_bind_info.protocol = "trpc";

  server_transport.Bind(bad_bind_info);
  bool listen_fail = server_transport.Listen();
  EXPECT_EQ(listen_fail, false);
}

TEST(FiberServerTransportImplTest, DoCloseTest) {
  FiberServerTransportImpl server_transport;

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  ConnectionPtr alive_conn;
  info.checker_function = [&alive_conn](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    alive_conn = conn;
    return PacketChecker::PACKET_FULL;
  };
  info.msg_handle_function = [&server_transport](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      context->SetNetType(ServerContext::NetType::kTcp);
      context->SetReserved(conn.Get());
      context->SetIp(conn->GetPeerIp());
      context->SetPort(conn->GetPeerPort());

      rsp->context = context;
      rsp->buffer = std::move(buff);

      server_transport.SendMsg(rsp);

      ++it;
    }

    return true;
  };
  info.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };
  FiberMutex mutex;
  FiberConditionVariable cond;
  bool is_closed = false;
  info.conn_close_function = [&cond, &mutex, &is_closed](const Connection*) {
    std::scoped_lock lk(mutex);
    is_closed = true;
    cond.notify_one();
  };

  server_transport.Bind(info);
  server_transport.Listen();

  NetworkAddress addr(info.ip, info.port, info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret = socket.Connect(addr);
  ASSERT_TRUE(ret == 0);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket.Send(message, sizeof(message));
  ASSERT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  socket.Recv(recvbuf, sizeof(recvbuf));

  ASSERT_STREQ(recvbuf, message);

  ASSERT_TRUE(server_transport.IsConnected(alive_conn->GetConnId()));

  CloseConnectionInfo close_info1;
  close_info1.connection_id = 23456;
  server_transport.DoClose(close_info1);
  ASSERT_FALSE(is_closed);

  CloseConnectionInfo close_info2;
  close_info2.connection_id = alive_conn->GetConnId();
  close_info2.fd = -1;
  server_transport.DoClose(close_info2);
  ASSERT_FALSE(is_closed);

  CloseConnectionInfo close_info3;
  close_info3.connection_id = alive_conn->GetConnId();
  close_info3.client_ip = alive_conn->GetPeerIp();
  close_info3.client_port = alive_conn->GetPeerPort();
  close_info3.fd = alive_conn->GetFd();
  server_transport.DoClose(close_info3);
  {
    std::unique_lock lk(mutex);
    cond.wait_for(lk, 10000ms, [&is_closed] { return is_closed; });
  }
  ASSERT_TRUE(is_closed);
  ASSERT_FALSE(server_transport.IsConnected(alive_conn->GetConnId()));

  server_transport.Stop();
  server_transport.Destroy();
}

}  // namespace trpc::testing

int Start(int argc, char** argv, std::function<int(int, char**)> cb) {
  signal(SIGPIPE, SIG_IGN);

  trpc::TrpcConfig::GetInstance()->Init("./trpc/runtime/threadmodel/testing/fiber.yaml");

  trpc::fiber::StartRuntime();

  // Now we start to run in fiber environment.
  int rc = 0;
  {
    trpc::Latch l(1);
    trpc::StartFiberDetached([&] {
      trpc::StartAllFiberReactor();

      rc = cb(argc, argv);

      trpc::StopAllFiberReactor();
      trpc::JoinAllFiberReactor();

      l.count_down();
    });
    l.wait();
  }

  trpc::fiber::TerminateRuntime();
  return rc;
}

int InitAndRunAllTests(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return Start(argc, argv, [](auto, auto) { return ::RUN_ALL_TESTS(); });
}

int main(int argc, char** argv) { return InitAndRunAllTests(argc, argv); }
