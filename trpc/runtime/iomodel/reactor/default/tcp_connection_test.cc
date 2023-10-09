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

#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"
#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_acceptor.h"
#include "trpc/util/net_util.h"
#include "trpc/util/thread/latch.h"
#include "trpc/util/time.h"

namespace trpc::testing {

class TcpConnectionHandler : public ConnectionHandler {
 public:
  explicit TcpConnectionHandler(Connection* conn, Reactor* reactor) : conn_(conn), reactor_(reactor) {}

  Connection* GetConnection() const override { return conn_; }

  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    std::cout << "TcpConnectionHandler is_client:" << conn->IsClient() << ", data:" << FlattenSlow(in)
              << ", total_buff_size:" << in.ByteSize() << std::endl;
    out.push_back(in);
    return kPacketFull;
  }

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override {
    std::cout << "TcpConnectionHandler is_client:" << conn->IsClient() << ", msg size:" << msg.size() << std::endl;
    auto it = msg.begin();
    while (it != msg.end()) {
      auto buff = std::any_cast<trpc::NoncontiguousBuffer&&>(std::move(*it));

      std::cout << "TcpConnectionHandler data:" << FlattenSlow(buff) << ",total_buff_size:" << buff.ByteSize()
                << std::endl;

      IoMessage io_message;
      io_message.ip = conn->GetPeerIp();
      io_message.port = conn->GetPeerPort();
      io_message.buffer = std::move(buff);

      conn->Send(std::move(io_message));

      ++it;
    }

    return true;
  }

 protected:
  Connection* conn_;
  Reactor* reactor_;
};

class ClientTcpConnectionHandler : public TcpConnectionHandler {
 public:
  ClientTcpConnectionHandler(Connection* conn, Reactor* reactor, int& ret, std::mutex& mutex,
                             std::condition_variable& cond)
      : TcpConnectionHandler(conn, reactor), ret_(ret), mutex_(mutex), cond_(cond) {}

  ~ClientTcpConnectionHandler() override {}

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override {
    EXPECT_EQ(msg.size(), 1);

    auto it = msg.begin();

    auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);
    std::cout << "ClientTcpConnectionHandler data:" << FlattenSlow(buff) << ",total_buff_size:" << buff.ByteSize()
              << std::endl;

    std::unique_lock<std::mutex> lock(mutex_);
    ret_ = 1;

    std::cout << "got reply and notify to end" << std::endl;
    cond_.notify_all();

    return true;
  }

 private:
  Reactor* reactor_;
  int& ret_;
  std::mutex& mutex_;
  std::condition_variable& cond_;
};

class TcpConnectionHandlerCloseAfterSend : public TcpConnectionHandler {
 public:
  explicit TcpConnectionHandlerCloseAfterSend(Connection* conn, Reactor* reactor)
      : TcpConnectionHandler(conn, reactor) {}

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override {
    std::cout << "TcpConnectionHandler is_client:" << conn->IsClient() << ", msg size:" << msg.size() << std::endl;
    auto it = msg.begin();
    while (it != msg.end()) {
      auto buff = std::any_cast<trpc::NoncontiguousBuffer&&>(std::move(*it));

      IoMessage io_message;
      io_message.ip = conn->GetPeerIp();
      io_message.port = conn->GetPeerPort();
      io_message.buffer = buff;

      conn->Send(std::move(io_message));

      ++it;
    }

    Reactor::Task server_close_task = [conn]() { conn->DoClose(false); };
    reactor_->SubmitTask(std::move(server_close_task));

    return true;
  }
};

class ClientTcpConnectTimeoutHandler : public TcpConnectionHandler {
 public:
  explicit ClientTcpConnectTimeoutHandler(Connection* conn, Reactor* reactor) : TcpConnectionHandler(conn, reactor) {}
  void ConnectionTimeout() override { connect_timeout_ = true; }

  static bool IsConnectTimeout() { return connect_timeout_; }

 private:
  static bool connect_timeout_;
};

bool ClientTcpConnectTimeoutHandler::connect_timeout_ = false;

class TcpConnectionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ReactorImpl::Options reactor_options;
    reactor_options.id = 0;

    reactor_ = std::make_unique<ReactorImpl>(reactor_options);
    reactor_->Initialize();

    Latch l(1);
    std::thread t([this, &l]() {
      l.count_down();
      this->reactor_->Run();
    });

    thread_ = std::move(t);

    l.wait();
  }

  void TearDown() override {
    reactor_->Stop();
    thread_.join();
    reactor_->Destroy();
  }

 protected:
  std::unique_ptr<Reactor> reactor_;
  std::thread thread_;
};

TEST_F(TcpConnectionTest, TcpConnectionSucc) {
  RefPtr<Connection> conn = nullptr;
  auto&& accept_handler = [this, &conn](AcceptConnectionInfo& connection_info) {
    connection_info.socket.SetTcpNoDelay();
    connection_info.socket.SetCloseWaitDefault();
    connection_info.socket.SetBlock(false);
    connection_info.socket.SetKeepAlive();
    conn = MakeRefCounted<TcpConnection>(this->reactor_.get(), connection_info.socket);
    conn->SetConnId(1);
    conn->SetConnType(ConnectionType::kTcpLong);
    conn->SetMaxPacketSize(10000000);
    conn->SetPeerIp(connection_info.conn_info.remote_addr.Ip());
    conn->SetPeerPort(connection_info.conn_info.remote_addr.Port());
    conn->SetPeerIpType(connection_info.conn_info.remote_addr.Type());
    conn->SetLocalIp(connection_info.conn_info.remote_addr.Ip());
    conn->SetLocalPort(connection_info.conn_info.remote_addr.Port());
    conn->SetLocalIpType(connection_info.conn_info.remote_addr.Type());

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(conn.Get());
    conn->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<TcpConnectionHandler>(conn.Get(), this->reactor_.get());
    conn->SetConnectionHandler(std::move(conn_handle));

    conn->Established();
    conn->StartHandshaking();

    return true;
  };

  NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV4);

  RefPtr<TcpAcceptor> acceptor = MakeRefCounted<TcpAcceptor>(reactor_.get(), addr);
  acceptor->SetAcceptHandleFunction(std::move(accept_handler));

  acceptor->EnableListen();

  std::cout << "listen" << std::endl;

  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  RefPtr<Connection> client_conn = nullptr;
  Reactor::Task reactor_task = [this, &ret, &mutex, &cond, &addr, &client_conn] {
    trpc::Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());
    socket.SetTcpNoDelay();
    socket.SetCloseWaitDefault();
    socket.SetBlock(false);
    socket.SetKeepAlive();
    client_conn = MakeRefCounted<TcpConnection>(this->reactor_.get(), socket);

    client_conn->SetConnId(2);
    client_conn->SetClient();
    client_conn->SetPeerIp(addr.Ip());
    client_conn->SetPeerPort(addr.Port());
    client_conn->SetPeerIpType(addr.Type());
    client_conn->SetConnType(ConnectionType::kTcpLong);

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(client_conn.Get());
    client_conn->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<ClientTcpConnectionHandler>(client_conn.Get(), this->reactor_.get(), ret, mutex, cond);
    client_conn->SetConnectionHandler(std::move(conn_handle));

    bool conn_flag = client_conn->DoConnect();

    ASSERT_EQ(1, client_conn->GetMergeRequestCount());

    ASSERT_EQ(conn_flag, true);

    std::string message("helloworld");

    IoMessage io_message;
    io_message.buffer = CreateBufferSlow(message.c_str(), message.size());

    std::cout << "send begin" << std::endl;
    client_conn->Send(std::move(io_message));
    std::cout << "send end" << std::endl;
  };

  std::cout << "before" << std::endl;

  reactor_->SubmitTask(std::move(reactor_task));

  std::cout << "after" << std::endl;

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }

  ConnectionState state = conn->GetConnectionState();

  ASSERT_EQ(state, ConnectionState::kConnected);

  ret = 0;

  Reactor::Task close_task = [this, &ret, &mutex, &cond, &client_conn] {
    client_conn->DoClose(true);

    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };

  reactor_->SubmitTask(std::move(close_task));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }

  ASSERT_EQ(ret, 1);

  ret = 0;
  Reactor::Task close_task2 = [&ret, &mutex, &cond, &conn, &acceptor] {
    conn->DoClose(true);

    acceptor->DisableListen();

    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };

  reactor_->SubmitTask(std::move(close_task2));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }

  std::cout << "end" << std::endl;
}

TEST_F(TcpConnectionTest, RemoteClose) {
  RefPtr<Connection> conn = nullptr;
  auto&& accept_handler = [this, &conn](const AcceptConnectionInfo& connection_info) {
    conn = MakeRefCounted<TcpConnection>(this->reactor_.get(), connection_info.socket);
    conn->SetConnId(1);
    conn->SetConnType(ConnectionType::kTcpLong);
    conn->SetMaxPacketSize(10000000);
    conn->SetPeerIp(connection_info.conn_info.remote_addr.Ip());
    conn->SetPeerPort(connection_info.conn_info.remote_addr.Port());
    conn->SetPeerIpType(connection_info.conn_info.remote_addr.Type());
    conn->SetLocalIp(connection_info.conn_info.remote_addr.Ip());
    conn->SetLocalPort(connection_info.conn_info.remote_addr.Port());
    conn->SetLocalIpType(connection_info.conn_info.remote_addr.Type());

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(conn.Get());
    conn->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<TcpConnectionHandlerCloseAfterSend>(conn.Get(), this->reactor_.get());
    conn->SetConnectionHandler(std::move(conn_handle));

    conn->Established();
    conn->StartHandshaking();

    return true;
  };

  NetworkAddress addr =
      NetworkAddress(trpc::util::GenRandomAvailablePort() + 1, false, NetworkAddress::IpType::kIpV4);

  RefPtr<TcpAcceptor> acceptor = MakeRefCounted<TcpAcceptor>(reactor_.get(), addr);
  acceptor->SetAcceptHandleFunction(std::move(accept_handler));

  acceptor->EnableListen();

  std::cout << "listen" << std::endl;

  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  RefPtr<Connection> client_conn = nullptr;
  Reactor::Task reactor_task = [this, &ret, &mutex, &cond, &addr, &client_conn] {
    trpc::Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());
    client_conn = MakeRefCounted<TcpConnection>(this->reactor_.get(), socket);

    client_conn->SetConnId(2);
    client_conn->SetClient();
    client_conn->SetPeerIp(addr.Ip());
    client_conn->SetPeerPort(addr.Port());
    client_conn->SetPeerIpType(addr.Type());
    client_conn->SetConnType(ConnectionType::kTcpLong);

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(client_conn.Get());
    client_conn->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<ClientTcpConnectionHandler>(client_conn.Get(), this->reactor_.get(), ret, mutex, cond);
    client_conn->SetConnectionHandler(std::move(conn_handle));

    bool conn_flag = client_conn->DoConnect();

    ASSERT_EQ(1, client_conn->GetMergeRequestCount());

    ASSERT_EQ(conn_flag, true);

    std::string message("helloworld");

    IoMessage io_message;
    io_message.buffer = CreateBufferSlow(message.c_str(), message.size());

    std::cout << "send begin" << std::endl;
    client_conn->Send(std::move(io_message));
    std::cout << "send end" << std::endl;
  };

  std::cout << "before" << std::endl;

  reactor_->SubmitTask(std::move(reactor_task));

  std::cout << "after" << std::endl;

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }

  std::cout << "wait end" << std::endl;

  ret = 0;
  Reactor::Task close_task = [&ret, &mutex, &cond, &conn, &acceptor] {
    ConnectionState state = conn->GetConnectionState();

    ASSERT_EQ(state, ConnectionState::kUnconnected);

    conn->DoClose(true);

    acceptor->DisableListen();

    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };

  reactor_->SubmitTask(std::move(close_task));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }

  ASSERT_EQ(ret, 1);
}

// Test the scenario with a connection timeout
TEST_F(TcpConnectionTest, ConnectTimeout) {
  NetworkAddress addr =
      NetworkAddress("255.255.255.1", trpc::util::GenRandomAvailablePort(), NetworkAddress::IpType::kIpV4);

  ConnectionInfo conn_peer_info;
  conn_peer_info.remote_addr = addr;

  trpc::Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());
  socket.SetTcpNoDelay();
  socket.SetCloseWaitDefault();
  socket.SetBlock(false);
  socket.SetKeepAlive();
  TcpConnection conn(this->reactor_.get(), socket);

  conn.SetConnId(1);
  conn.SetConnType(ConnectionType::kTcpLong);
  conn.SetPeerIp(addr.Ip());
  conn.SetPeerPort(addr.Port());
  conn.SetPeerIpType(NetworkAddress::IpType::kIpV4);
  conn.SetCheckConnectTimeout(200);

  std::unique_ptr<ConnectionHandler> conn_handle =
      std::make_unique<ClientTcpConnectTimeoutHandler>(&conn, this->reactor_.get());
  conn.SetConnectionHandler(std::move(conn_handle));
  std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(&conn);
  conn.SetIoHandler(std::move(io_handle));

  uint64_t start_time = trpc::time::GetMilliSeconds();
  conn.DoConnect();

  ASSERT_TRUE(conn.GetConnectionState() == ConnectionState::kConnecting);

  std::cout << "wait" << std::endl;

  // Wait for a maximum of 5 seconds.
  int loop_times = 0;
  while (!ClientTcpConnectTimeoutHandler::IsConnectTimeout() && loop_times < 10) {
    usleep(500000);
    loop_times++;
  }

  std::cout << "wait end" << std::endl;

  ASSERT_TRUE(ClientTcpConnectTimeoutHandler::IsConnectTimeout());
  uint64_t end_time = trpc::time::GetMilliSeconds();
  ASSERT_TRUE(end_time - start_time >= conn.GetCheckConnectTimeout());
  ASSERT_TRUE(conn.GetConnectionState() == ConnectionState::kUnconnected);
}

TEST_F(TcpConnectionTest, DisableRead) {
  RefPtr<TcpConnection> conn = nullptr;
  auto&& accept_handler = [this, &conn](AcceptConnectionInfo& connection_info) {
    connection_info.socket.SetTcpNoDelay();
    connection_info.socket.SetCloseWaitDefault();
    connection_info.socket.SetBlock(false);
    connection_info.socket.SetKeepAlive();

    conn = MakeRefCounted<TcpConnection>(this->reactor_.get(), connection_info.socket);
    conn->SetConnId(1);
    conn->SetConnType(ConnectionType::kTcpLong);
    conn->SetMaxPacketSize(10000000);
    conn->SetPeerIp(connection_info.conn_info.remote_addr.Ip());
    conn->SetPeerPort(connection_info.conn_info.remote_addr.Port());
    conn->SetPeerIpType(connection_info.conn_info.remote_addr.Type());
    conn->SetLocalIp(connection_info.conn_info.remote_addr.Ip());
    conn->SetLocalPort(connection_info.conn_info.remote_addr.Port());
    conn->SetLocalIpType(connection_info.conn_info.remote_addr.Type());

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(conn.Get());
    conn->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<TcpConnectionHandler>(conn.Get(), this->reactor_.get());
    conn->SetConnectionHandler(std::move(conn_handle));

    conn->Established();
    conn->StartHandshaking();

    // Disabling server connection read event processing, the client will no longer receive a reply After.
    conn->DisableRead();

    return true;
  };

  NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV4);

  RefPtr<TcpAcceptor> acceptor = MakeRefCounted<TcpAcceptor>(reactor_.get(), addr);
  acceptor->SetAcceptHandleFunction(std::move(accept_handler));
  acceptor->EnableListen();

  std::cout << "listen" << std::endl;

  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  RefPtr<Connection> client_conn = nullptr;
  Reactor::Task reactor_task = [this, &ret, &mutex, &cond, &addr, &client_conn] {
    trpc::Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());
    socket.SetTcpNoDelay();
    socket.SetCloseWaitDefault();
    socket.SetBlock(false);
    socket.SetKeepAlive();

    client_conn = MakeRefCounted<TcpConnection>(this->reactor_.get(), socket);

    client_conn->SetConnId(2);
    client_conn->SetClient();
    client_conn->SetPeerIp(addr.Ip());
    client_conn->SetPeerPort(addr.Port());
    client_conn->SetPeerIpType(addr.Type());
    client_conn->SetConnType(ConnectionType::kTcpLong);

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(client_conn.Get());
    client_conn->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<ClientTcpConnectionHandler>(client_conn.Get(), this->reactor_.get(), ret, mutex, cond);
    client_conn->SetConnectionHandler(std::move(conn_handle));

    ASSERT_EQ(1, client_conn->GetMergeRequestCount());

    bool conn_flag = client_conn->DoConnect();
    ASSERT_EQ(conn_flag, true);

    std::string message("helloworld");

    IoMessage io_message;
    io_message.buffer = CreateBufferSlow(message.c_str(), message.size());

    std::cout << "send begin" << std::endl;
    int succ = client_conn->Send(std::move(io_message));
    std::cout << "send end" << std::endl;

    ASSERT_EQ(succ, 0);
  };

  std::cout << "before" << std::endl;

  reactor_->SubmitTask(std::move(reactor_task));

  std::cout << "wait" << std::endl;

  // Wait for a maximum of 5 seconds. After 5 seconds, the connection is normal, but no reply is received.
  int loop_times = 0;
  while (!ret && loop_times < 10) {
    usleep(500000);
    loop_times++;
  }

  std::cout << "wait end" << std::endl;

  ConnectionState state = conn->GetConnectionState();

  // The connection is normal, but no response (rsp) was received, and the return value (ret) is still 0.
  ASSERT_EQ(state, ConnectionState::kConnected);
  ASSERT_EQ(ret, 0);

  ret = 0;
  Reactor::Task close_task = [this, &ret, &mutex, &cond, &client_conn] {
    client_conn->DoClose(true);

    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };

  reactor_->SubmitTask(std::move(close_task));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }

  ASSERT_EQ(ret, 1);

  ret = 0;
  Reactor::Task close_task2 = [this, &ret, &mutex, &cond, &conn] {
    conn->DoClose(true);

    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };

  reactor_->SubmitTask(std::move(close_task2));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }

  std::cout << "end" << std::endl;
}

// Testing the scenario where the IO message queue exceeds the limit while in the 'connecting' state.
TEST_F(TcpConnectionTest, ConnectingIoMsgLimited) {
  Latch latch(1);
  this->reactor_->SubmitTask([this, &latch]() {
    NetworkAddress addr =
        NetworkAddress("255.255.255.1", trpc::util::GenRandomAvailablePort(), NetworkAddress::IpType::kIpV4);

    ConnectionInfo conn_peer_info;
    conn_peer_info.remote_addr = addr;

    trpc::Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());
    socket.SetTcpNoDelay();
    socket.SetCloseWaitDefault();
    socket.SetBlock(false);
    socket.SetKeepAlive();
    TcpConnection conn(this->reactor_.get(), socket);

    conn.SetConnId(1);
    conn.SetConnType(ConnectionType::kTcpLong);
    conn.SetPeerIp(addr.Ip());
    conn.SetPeerPort(addr.Port());
    conn.SetPeerIpType(NetworkAddress::IpType::kIpV4);

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<ClientTcpConnectTimeoutHandler>(&conn, this->reactor_.get());
    conn.SetConnectionHandler(std::move(conn_handle));
    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(&conn);
    conn.SetIoHandler(std::move(io_handle));

    conn.DoConnect();

    std::string message("helloworld");
    for (uint32_t i = 0; i < 1024; i++) {
      ASSERT_TRUE(conn.GetConnectionState() == ConnectionState::kConnecting);
      IoMessage io_message;
      io_message.buffer = CreateBufferSlow(message.c_str(), message.size());

      ASSERT_TRUE(conn.Send(std::move(io_message)) == 0);
    }

    // IO message queue exceeds the limit
    IoMessage io_message;
    io_message.buffer = CreateBufferSlow(message.c_str(), message.size());

    ASSERT_TRUE(conn.Send(std::move(io_message)) == -1);

    conn.DoClose(true);

    latch.count_down();
  });

  latch.wait();
}

}  // namespace trpc::testing
