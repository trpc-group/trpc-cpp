//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/native/stream_connection_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"

#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_acceptor.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/util/latch.h"
#include "trpc/util/net_util.h"

namespace trpc {

namespace testing {

class ConnectionTestHandler : public ConnectionHandler {
 public:
  using Callback = Function<void()>;

  explicit ConnectionTestHandler(size_t size, Connection* conn)
      : size_(size), conn_(conn) {}

  Connection* GetConnection() const override { return conn_; }

  int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    uint32_t total_buff_size = in.ByteSize();

    if (total_buff_size < size_) {
      return kPacketLess;
    }

    std::cout << "data:" << FlattenSlow(in) << ",total_buff_size:" << total_buff_size << std::endl;

    return kPacketFull;
  }

  bool HandleMessage(const ConnectionPtr&, std::deque<std::any>&) override {
    if (cb_) {
      cb_();
    }
    return true;
  }

  void SetHandleFunc(Callback&& cb) { cb_ = std::move(cb); }

 private:
  size_t size_;
  Connection* conn_;
  Callback cb_;
};

constexpr size_t kDataSize = 100;

class FiberTcpConnectionTestImpl {
 public:
  void SetUp() {
    reactor_ = trpc::fiber::GetReactor(0, -2);

    addr_ = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV4);

    acceptor_ = MakeRefCounted<FiberAcceptor>(reactor_, addr_);
    trpc::AcceptConnectionFunction accept_handler = [this](AcceptConnectionInfo& connection_info) {
      connection_info.socket.SetTcpNoDelay();
      connection_info.socket.SetCloseWaitDefault();
      connection_info.socket.SetBlock(false);
      connection_info.socket.SetKeepAlive();
      server_conn_ = MakeRefCounted<FiberTcpConnection>(reactor_, connection_info.socket);

      server_conn_->SetLocalIp(connection_info.conn_info.local_addr.Ip());
      server_conn_->SetLocalPort(connection_info.conn_info.local_addr.Port());
      server_conn_->SetLocalIpType(connection_info.conn_info.local_addr.Type());

      auto handler = std::make_unique<ConnectionTestHandler>(kDataSize, server_conn_.Get());
      auto cb = [this] {
        server_received_ += kDataSize;

        IoMessage msg;
        msg.seq_id = 0;
        msg.buffer = CreateBufferSlow(std::string(kDataSize, 1));
        server_conn_->Send(std::move(msg));
      };
      handler.get()->SetHandleFunc(std::move(cb));
      server_conn_->SetConnectionHandler(std::move(handler));

      auto io_handle = std::make_unique<DefaultIoHandler>(server_conn_.Get());
      server_conn_->SetIoHandler(std::move(io_handle));

      server_conn_->Established();
      server_conn_->StartHandshaking();

      return true;
    };
    acceptor_->SetAcceptHandleFunction(std::move(accept_handler));
    std::cout << "fiber acceptor listen" << std::endl;
    acceptor_->Listen();
  }

  void TearDown() {
    if (server_conn_) {
      server_conn_->Stop();
      server_conn_->Join();
    }

    server_conn_ = nullptr;

    acceptor_->Stop();
    acceptor_->Join();
  }

  template <class IoHandlerType>
  RefPtr<FiberTcpConnection> CreateClientConn() {
    trpc::Socket socket = Socket::CreateTcpSocket(addr_.IsIpv6());
    socket.SetTcpNoDelay();
    socket.SetCloseWaitDefault();
    socket.SetBlock(false);
    socket.SetKeepAlive();

    RefPtr<FiberTcpConnection> client_conn = MakeRefCounted<FiberTcpConnection>(reactor_, socket);
    client_conn->SetPeerIp(addr_.Ip());
    client_conn->SetPeerPort(addr_.Port());
    client_conn->SetPeerIpType(addr_.Type());
    client_conn->SetClient();

    std::unique_ptr<ConnectionTestHandler> handler = std::make_unique<ConnectionTestHandler>(kDataSize, client_conn.Get());
    auto cb = [this] { client_received_ += kDataSize; };
    handler->SetHandleFunc(std::move(cb));
    client_conn->SetConnectionHandler(std::move(handler));

    std::unique_ptr<IoHandler> io_handler = std::make_unique<IoHandlerType>(client_conn.Get());
    client_conn->SetIoHandler(std::move(io_handler));

    bool flag = client_conn->DoConnect();
    TRPC_ASSERT(flag == true);

    client_conn->StartHandshaking();

    return client_conn;
  }

  std::size_t GetServerReceived() { return server_received_.load(); }

  std::size_t GetClientReceived() { return client_received_.load(); }

 protected:
  Reactor* reactor_;
  NetworkAddress addr_;
  RefPtr<FiberAcceptor> acceptor_{nullptr};
  RefPtr<FiberTcpConnection> server_conn_{nullptr};
  std::atomic<std::size_t> server_received_{0};
  std::atomic<std::size_t> client_received_{0};
};

class FiberTcpConnectionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() { test_impl_.SetUp(); }
  static void TearDownTestCase() { test_impl_.TearDown(); }

  template <class IoHandlerType>
  RefPtr<FiberTcpConnection> CreateClientConn() {
    return test_impl_.CreateClientConn<IoHandlerType>();
  }

  std::size_t GetServerReceived() { return test_impl_.GetServerReceived(); }

  std::size_t GetClientReceived() { return test_impl_.GetClientReceived(); }

 protected:
  static FiberTcpConnectionTestImpl test_impl_;
};

FiberTcpConnectionTestImpl FiberTcpConnectionTest::test_impl_;

TEST_F(FiberTcpConnectionTest, OK) {
  std::cout << "create client fiber connection" << std::endl;
  RefPtr<FiberTcpConnection> client_conn = CreateClientConn<DefaultIoHandler>();

  IoMessage msg;
  msg.seq_id = 0;
  msg.buffer = CreateBufferSlow(std::string(kDataSize, 1));

  client_conn->Send(std::move(msg));

  while (GetServerReceived() != kDataSize) {
    FiberSleepFor(std::chrono::milliseconds(1));
  }

  while (GetClientReceived() != kDataSize) {
    FiberSleepFor(std::chrono::milliseconds(1));
  }

  ASSERT_EQ(GetClientReceived(), kDataSize);

  client_conn->DoClose(false);

  client_conn->Stop();
  client_conn->Join();
}

TEST_F(FiberTcpConnectionTest, WriteEmpty) {
  std::cout << "create client fiber connection" << std::endl;
  RefPtr<FiberTcpConnection> client_conn = CreateClientConn<DefaultIoHandler>();

  IoMessage msg;
  msg.seq_id = 0;
  msg.buffer = {};

  EXPECT_EQ(0, client_conn->Send(std::move(msg)));
}

class WriteErrorIoHandler : public DefaultIoHandler {
 public:
  explicit WriteErrorIoHandler(Connection* conn) : DefaultIoHandler(conn) {}

  MOCK_METHOD(int, Writev, (const iovec* iov, int iovcnt), (override));
};

TEST_F(FiberTcpConnectionTest, WriteError1) {
  RefPtr<FiberTcpConnection> client_conn = CreateClientConn<WriteErrorIoHandler>();

  IoMessage msg;
  msg.seq_id = 0;
  msg.buffer = CreateBufferSlow(std::string(kDataSize, 1));

  EXPECT_CALL(*static_cast<WriteErrorIoHandler*>(client_conn->GetIoHandler()),
              Writev(::testing::_, ::testing::_))
      .WillOnce(::testing::Return(-1));
  int ret = client_conn->Send(std::move(msg));
  ASSERT_EQ(ret, -1);

  client_conn->Stop();
  client_conn->Join();
}

TEST_F(FiberTcpConnectionTest, WriteError2) {
  RefPtr<FiberTcpConnection> client_conn = CreateClientConn<WriteErrorIoHandler>();

  IoMessage msg;
  msg.seq_id = 0;
  msg.buffer = CreateBufferSlow(std::string(kDataSize, 1));

  EXPECT_CALL(*static_cast<WriteErrorIoHandler*>(client_conn->GetIoHandler()),
              Writev(::testing::_, ::testing::_))
      .WillOnce(::testing::Return(0));
  int ret = client_conn->Send(std::move(msg));
  ASSERT_EQ(ret, -1);

  client_conn->Stop();
  client_conn->Join();
}

}  // namespace testing

}  // namespace trpc

int Start(int argc, char** argv, std::function<int(int, char**)> cb) {
  signal(SIGPIPE, SIG_IGN);

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
