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

#include "trpc/runtime/iomodel/reactor/default/udp_transceiver.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"
#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/util/net_util.h"
#include "trpc/util/thread/latch.h"

namespace trpc {

namespace testing {

class UdpConnectionHandler : public ConnectionHandler {
 public:
  explicit UdpConnectionHandler(Connection* conn, Reactor* reactor) : conn_(conn), reactor_(reactor) {}

  ~UdpConnectionHandler() override {}

  Connection* GetConnection() const override { return conn_; }

  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    std::cout << "UdpConnectionHandler data:" << FlattenSlow(in) << ",total_buff_size:" << in.ByteSize() << std::endl;
    out.push_back(in);
    return kPacketFull;
  }

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override {
    ConnectionPtr connection = const_pointer_cast<Connection>(conn);
    auto it = msg.begin();
    while (it != msg.end()) {
      auto buff = std::any_cast<trpc::NoncontiguousBuffer&&>(std::move(*it));

      IoMessage io_message;
      io_message.ip = conn->GetPeerIp();
      io_message.port = conn->GetPeerPort();
      io_message.buffer = std::move(buff);

      std::cout << "UdpConnectionHandler data:" << FlattenSlow(buff) << ",total_buff_size:" << buff.ByteSize()
                << std::endl;

      connection->Send(std::move(io_message));

      ++it;
    }

    return true;
  }

 private:
  Connection* conn_;
  Reactor* reactor_;
};

class ClientUdpConnectionHandler : public UdpConnectionHandler {
 public:
  ClientUdpConnectionHandler(Connection* conn, Reactor* reactor, int& ret, std::mutex& mutex,
                             std::condition_variable& cond)
      : UdpConnectionHandler(conn, reactor), ret_(ret), mutex_(mutex), cond_(cond) {}

  ~ClientUdpConnectionHandler() override {}

  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    std::cout << "ClientUdpConnectionHandler data:" << FlattenSlow(in) << ",total_buff_size:" << in.ByteSize()
              << std::endl;
    out.push_back(in);
    return kPacketFull;
  }

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override {
    auto it = msg.begin();

    auto& buff = std::any_cast<NoncontiguousBuffer&>(*it);

    std::cout << "ClientUdpConnectionHandler data:" << FlattenSlow(buff) << ",total_buff_size:" << buff.ByteSize()
              << std::endl;

    std::unique_lock<std::mutex> lock(mutex_);
    ret_ = 1;
    std::cout << "MessageHandle" << std::endl;
    cond_.notify_all();

    return true;
  }

 private:
  int& ret_;
  std::mutex& mutex_;
  std::condition_variable& cond_;
};

class UdptransceiverTest : public ::testing::Test {
 public:
  void SetUp() override {
    ReactorImpl::Options reactor_options;
    reactor_options.id = 0;

    reactor_ = std::make_unique<ReactorImpl>(reactor_options);
    reactor_->Initialize();
  }

  void TearDown() override { reactor_->Destroy(); }

 protected:
  std::unique_ptr<Reactor> reactor_;
};

TEST_F(UdptransceiverTest, Send) {
  Latch l(1);
  std::thread t1([this, &l]() {
    l.count_down();
    this->reactor_->Run();
  });

  l.wait();

  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;

  RefPtr<UdpTransceiver> udp_transceiver = nullptr;

  NetworkAddress addr =
      NetworkAddress("127.0.0.1", trpc::util::GenRandomAvailablePort(), NetworkAddress::IpType::kIpV4);
  Reactor::Task reactor_task = [this, &ret, &mutex, &cond, &addr, &udp_transceiver] {
    trpc::Socket socket = Socket::CreateUdpSocket(addr.IsIpv6());
    socket.SetReuseAddr();
    socket.SetReusePort();
    socket.SetBlock(false);
    socket.Bind(addr);
    udp_transceiver = MakeRefCounted<UdpTransceiver>(this->reactor_.get(), socket);

    udp_transceiver->SetConnId(1);
    udp_transceiver->SetConnType(ConnectionType::kUdp);
    udp_transceiver->SetLocalIp(addr.Ip());
    udp_transceiver->SetLocalIpType(addr.Type());
    udp_transceiver->SetLocalPort(addr.Port());
    std::cout << "server port:" << addr.Port() << std::endl;

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(udp_transceiver.Get());
    udp_transceiver->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle =
        std::make_unique<UdpConnectionHandler>(udp_transceiver.Get(), this->reactor_.get());
    udp_transceiver->SetConnectionHandler(std::move(conn_handle));

    udp_transceiver->EnableReadWrite();
    udp_transceiver->StartHandshaking();

    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };

  this->reactor_->SubmitTask(std::move(reactor_task));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) cond.wait(lock);
  }
  ASSERT_EQ(ret, 1);

  std::cout << "ret:" << ret << std::endl;

  ret = 0;

  RefPtr<UdpTransceiver> client_udp_transceiver = nullptr;
  Reactor::Task client_task = [this, &ret, &mutex, &cond, &addr, &client_udp_transceiver] {
    trpc::Socket client_socket = Socket::CreateUdpSocket(addr.IsIpv6());

    client_socket.SetBlock(false);
    std::cout << "client fd:" << client_socket.GetFd() << std::endl;
    client_udp_transceiver = MakeRefCounted<UdpTransceiver>(this->reactor_.get(), client_socket);

    client_udp_transceiver->SetConnId(2);
    client_udp_transceiver->SetConnType(ConnectionType::kUdp);
    client_udp_transceiver->SetPeerIp(addr.Ip());
    client_udp_transceiver->SetPeerIpType(addr.Type());
    client_udp_transceiver->SetPeerPort(addr.Port());
    client_udp_transceiver->SetClient();

    std::unique_ptr<IoHandler> io_handle = std::make_unique<DefaultIoHandler>(client_udp_transceiver.Get());
    client_udp_transceiver->SetIoHandler(std::move(io_handle));

    std::unique_ptr<ConnectionHandler> conn_handle = std::make_unique<ClientUdpConnectionHandler>(
        client_udp_transceiver.Get(), this->reactor_.get(), ret, mutex, cond);
    client_udp_transceiver->SetConnectionHandler(std::move(conn_handle));

    client_udp_transceiver->EnableReadWrite();
    client_udp_transceiver->StartHandshaking();

    IoMessage io_message;
    io_message.ip = addr.Ip();
    io_message.port = addr.Port();
    io_message.buffer = CreateBufferSlow("helloworld");

    ASSERT_EQ(0, client_udp_transceiver->Send(std::move(io_message)));
    std::cout << "Send To " << addr.Ip() << ":" << addr.Port() << " , ret:" << ret << std::endl;

    // Sending an empty packet should return directly without going through the actual sending logic
    IoMessage empty_message;
    empty_message.ip = addr.Ip();
    empty_message.port = addr.Port();
    empty_message.buffer = CreateBufferSlow("");

    ASSERT_EQ(-1, client_udp_transceiver->Send(std::move(empty_message)));

    // Sending a packet of 64K should return an error directly
    IoMessage over_len_message;
    over_len_message.ip = addr.Ip();
    over_len_message.port = addr.Port();
    std::vector<char> data;
    data.assign(65535, 'a');
    over_len_message.buffer = CreateBufferSlow(static_cast<char*>(&data[0]), data.size());

    ASSERT_EQ(-1, client_udp_transceiver->Send(std::move(over_len_message)));
  };

  this->reactor_->SubmitTask(std::move(client_task));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret != 1) {
      cond.wait(lock);
    }
  }
  ASSERT_EQ(ret, 1);

  ret = 0;

  Reactor::Task close_task = [this, &ret, &mutex, &cond, &udp_transceiver, &client_udp_transceiver] {
    client_udp_transceiver->DisableReadWrite();
    udp_transceiver->DisableReadWrite();

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

  reactor_->Stop();

  t1.join();
}

}  // namespace testing

}  // namespace trpc
