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

#include "trpc/transport/server/default/server_transport_impl.h"

#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/net_util.h"

namespace trpc::testing {

class ServerTransportImplTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    int ret = TrpcConfig::GetInstance()->Init("./trpc/runtime/threadmodel/testing/separate.yaml");
    ASSERT_EQ(ret, 0);

    trpc::separate::StartRuntime();

    thread_model = static_cast<trpc::SeparateThreadModel*>(ThreadModelManager::GetInstance()->Get("default_instance"));
  }

  static void TearDownTestCase() {
    trpc::separate::TerminateRuntime();
  }

 public:
  static trpc::SeparateThreadModel* thread_model;
};

trpc::SeparateThreadModel* ServerTransportImplTest::thread_model = nullptr;

TEST_F(ServerTransportImplTest, Tcp) {
  std::vector<Reactor*> reactors = thread_model->GetReactors();
  ServerTransportImpl server_transport(std::move(reactors));

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
  EXPECT_TRUE(ret == 0);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket.Send(message, sizeof(message));
  EXPECT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  socket.Recv(recvbuf, sizeof(recvbuf));

  EXPECT_STREQ(recvbuf, message);

  ASSERT_TRUE(server_transport.IsConnected(alive_conn->GetConnId()));

  server_transport.Stop();
  server_transport.Destroy();
}

TEST_F(ServerTransportImplTest, Udp) {
  std::vector<Reactor*> reactors = thread_model->GetReactors();
  ServerTransportImpl server_transport(std::move(reactors));

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
  info.custom_set_accept_socket_opt_function = [](Socket& s) {};
  info.msg_handle_function = [&](const ConnectionPtr& conn, std::deque<std::any>& msg) {
    EXPECT_TRUE(msg.size() > 0);

    auto it = msg.begin();
    while (it != msg.end()) {
      auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
      auto* rsp = trpc::object_pool::New<STransportRspMsg>();

      trpc::ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
      context->SetConnectionId(conn->GetConnId());
      context->SetNetType(ServerContext::NetType::kUdp);
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
  bool ret = server_transport.Listen();
  EXPECT_EQ(ret, true);

  NetworkAddress addr(info.ip, info.port, info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket socket = Socket::CreateUdpSocket(addr.IsIpv6());

  char message[] = "helloworld, this is tcp,udp client";
  int send_size = socket.SendTo(message, sizeof(message), 0, addr);
  EXPECT_TRUE(send_size > 0);

  std::cout << "send ip:" << addr.Ip() << ",port:" << addr.Port() << std::endl;

  NetworkAddress peer_addr;
  char recvbuf[1024] = {0};
  socket.RecvFrom(recvbuf, sizeof(recvbuf), 0, &peer_addr);

  std::cout << "recv end" << std::endl;

  EXPECT_STREQ(recvbuf, message);

  server_transport.Stop();
  server_transport.Destroy();
}

TEST_F(ServerTransportImplTest, Uds) {
  std::vector<Reactor*> reactors = thread_model->GetReactors();
  ServerTransportImpl server_transport(std::move(reactors));

  BindInfo info;
  info.unix_path = "trpc_unix_test.socket";
  info.is_ipv6 = false;
  info.socket_type = "unix";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp";
  info.protocol = "raw";
  info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    EXPECT_TRUE(in.ByteSize() > 0);
    out.push_back(in);
    return PacketChecker::PACKET_FULL;
  };
  info.custom_set_accept_socket_opt_function = [](Socket& s) {};
  info.msg_handle_function = [&](const ConnectionPtr& conn, std::deque<std::any>& msg) {
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

  server_transport.Bind(info);
  server_transport.Listen();

  auto socket = std::make_unique<Socket>(Socket::CreateUnixSocket());

  UnixAddress addr(info.unix_path.c_str());
  socket->Connect(addr);

  char message[] = "helloworld, this is unix client";
  int send_size = socket->Send(message, sizeof(message));
  EXPECT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  socket->Recv(recvbuf, sizeof(recvbuf));

  EXPECT_STREQ(recvbuf, message);

  server_transport.Stop();
  server_transport.Destroy();
}

TEST_F(ServerTransportImplTest, TcpUdp) {
  std::vector<Reactor*> reactors = thread_model->GetReactors();
  ServerTransportImpl server_transport(std::move(reactors));

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp,udp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  info.dispatch_accept_function = [](const AcceptConnectionInfo& info, std::size_t total_io_num) { return 0; };
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
      if (conn->GetConnType() == ConnectionType::kTcpLong) {
        context->SetNetType(ServerContext::NetType::kTcp);
      } else if (conn->GetConnType() == ConnectionType::kUdp) {
        context->SetNetType(ServerContext::NetType::kUdp);
      } else {
        assert(false);
      }
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

  Socket tcp_socket = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret = tcp_socket.Connect(addr);
  EXPECT_TRUE(ret == 0);

  char tcp_message[] = "helloworld, this is tcp client";
  int tcp_send_size = tcp_socket.Send(tcp_message, sizeof(tcp_message));
  EXPECT_TRUE(tcp_send_size > 0);

  char tcp_recvbuf[1024] = {0};
  tcp_socket.Recv(tcp_recvbuf, sizeof(tcp_recvbuf));

  EXPECT_STREQ(tcp_recvbuf, tcp_message);

  ASSERT_TRUE(server_transport.IsConnected(alive_conn->GetConnId()));

  Socket udp_socket = Socket::CreateUdpSocket(addr.IsIpv6());

  char udp_message[] = "helloworld, this is tcp,udp client";
  int udp_send_size = udp_socket.SendTo(udp_message, sizeof(udp_message), 0, addr);
  EXPECT_TRUE(udp_send_size > 0);

  NetworkAddress peer_addr;
  char udp_recvbuf[1024] = {0};
  udp_socket.RecvFrom(udp_recvbuf, sizeof(udp_recvbuf), 0, &peer_addr);

  EXPECT_STREQ(udp_recvbuf, udp_message);

  server_transport.Stop();
  server_transport.Destroy();
}

TEST_F(ServerTransportImplTest, TcpBindFail) {
  std::vector<Reactor*> reactors = thread_model->GetReactors();
  ServerTransportImpl server_transport(std::move(reactors));

  BindInfo bad_info;
  bad_info.is_ipv6 = false;
  bad_info.socket_type = "net";
  // invalid ip 1.1.1.1
  bad_info.ip = "1.1.1.1";
  bad_info.port = 10000;
  bad_info.network = "tcp";
  server_transport.Bind(bad_info);
  bool listen_fail = server_transport.Listen();
  EXPECT_EQ(listen_fail, false);
}

TEST_F(ServerTransportImplTest, StopListen) {
  std::vector<Reactor*> reactors = thread_model->GetReactors();
  ServerTransportImpl server_transport(std::move(reactors));

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

  Socket socket1 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret1 = socket1.Connect(addr);
  EXPECT_TRUE(ret1 == 0);

  server_transport.StopListen(false);

  ::usleep(10000);

  Socket socket2 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret2 = socket2.Connect(addr);
  EXPECT_TRUE(ret2 == -1);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket1.Send(message, sizeof(message));
  EXPECT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  socket1.Recv(recvbuf, sizeof(recvbuf));

  EXPECT_STREQ(recvbuf, message);

  ASSERT_TRUE(server_transport.IsConnected(alive_conn->GetConnId()));

  socket1.Close();
  socket2.Close();

  server_transport.Stop();
  server_transport.Destroy();
}

TEST_F(ServerTransportImplTest, StopListenWithClean) {
  std::vector<Reactor*> reactors = thread_model->GetReactors();
  ServerTransportImpl server_transport(std::move(reactors));

  BindInfo info;
  info.is_ipv6 = false;
  info.socket_type = "net";
  info.ip = "0.0.0.0";
  info.port = trpc::util::GenRandomAvailablePort();
  info.network = "tcp";
  info.protocol = "raw";
  info.accept_function = nullptr;
  ConnectionPtr alive_conn = nullptr;
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

  Socket socket1 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret1 = socket1.Connect(addr);
  EXPECT_TRUE(ret1 == 0);

  struct timeval timeout = {3, 0};
  ret1 = socket1.SetSockOpt(SO_RCVTIMEO, static_cast<const void*>(&timeout),
                  static_cast<socklen_t>(sizeof(timeout)), SOL_SOCKET);
  EXPECT_TRUE(ret1 == 0);

  server_transport.StopListen(true);

  ::usleep(10000);

  Socket socket2 = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret2 = socket2.Connect(addr);
  EXPECT_TRUE(ret2 == -1);

  char message[] = "helloworld, this is tcp client";
  int send_size = socket1.Send(message, sizeof(message));
  EXPECT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  int recv_size = socket1.Recv(recvbuf, sizeof(recvbuf));
  EXPECT_TRUE(recv_size < 0);

  EXPECT_EQ(alive_conn, nullptr);

  socket1.Close();
  socket2.Close();

  server_transport.Stop();
  server_transport.Destroy();
}

}  // namespace trpc::testing
