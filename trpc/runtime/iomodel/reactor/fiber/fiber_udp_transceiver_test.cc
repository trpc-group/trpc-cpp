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

#include "trpc/runtime/iomodel/reactor/fiber/fiber_udp_transceiver.h"

#include <random>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/latch.h"
#include "trpc/util/net_util.h"

namespace trpc::testing {

class ConnectionTestHandler : public ConnectionHandler {
 public:
  using Callback = Function<void()>;

  explicit ConnectionTestHandler(size_t size, Connection* conn)
      : size_(size), conn_(conn) {}

  ~ConnectionTestHandler() override {}

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
    if (cb_) cb_();
    return true;
  }

  void SetHandleFunc(Callback&& cb) { cb_ = std::move(cb); }

 private:
  size_t size_;
  Connection* conn_;
  Callback cb_;
};

TEST(TestFiberUdpTransceiver, Normal) {
  size_t kDataSize = 100;
  std::atomic<std::size_t> server_received = 0;
  std::atomic<std::size_t> client_received = 0;

  Reactor* reactor = trpc::fiber::GetReactor(0, -2);
  NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV4);

  // Listening for UDP on the server side
  trpc::Socket socket = Socket::CreateUdpSocket(addr.IsIpv6());
  socket.SetReuseAddr();
  socket.SetReusePort();
  socket.SetBlock(false);
  socket.Bind(addr);

  auto udp_transceiver = MakeRefCounted<FiberUdpTransceiver>(reactor, socket);
  auto handler = std::make_unique<ConnectionTestHandler>(kDataSize, udp_transceiver.Get());
  auto cb = [&kDataSize, &server_received, &udp_transceiver] {
    server_received += kDataSize;

    IoMessage msg;
    msg.seq_id = 0;
    msg.ip = udp_transceiver->GetPeerIp();
    msg.port = udp_transceiver->GetPeerPort();
    msg.buffer = CreateBufferSlow(std::string(kDataSize, 1));
    udp_transceiver->Send(std::move(msg));
  };
  handler->SetHandleFunc(std::move(cb));
  udp_transceiver->SetConnectionHandler(std::move(handler));

  auto io_handle = std::make_unique<DefaultIoHandler>(udp_transceiver.Get());
  udp_transceiver->SetIoHandler(std::move(io_handle));
  udp_transceiver->SetLocalIp(addr.Ip());
  udp_transceiver->SetLocalPort(addr.Port());
  udp_transceiver->SetLocalIpType(addr.Type());

  udp_transceiver->EnableReadWrite();

  // Start a UDP client for testing
  trpc::Socket client_socket = Socket::CreateUdpSocket(addr.IsIpv6());
  client_socket.SetBlock(false);

  auto client_udp_transceiver = MakeRefCounted<FiberUdpTransceiver>(reactor, client_socket);
  client_udp_transceiver->SetPeerIp(addr.Ip());
  client_udp_transceiver->SetPeerPort(addr.Port());
  client_udp_transceiver->SetPeerIpType(addr.Type());
  client_udp_transceiver->SetClient();

  auto client_handler = std::make_unique<ConnectionTestHandler>(kDataSize, client_udp_transceiver.Get());
  auto client_cb = [&kDataSize, &client_received] { client_received += kDataSize; };
  client_handler->SetHandleFunc(std::move(client_cb));
  client_udp_transceiver->SetConnectionHandler(std::move(client_handler));

  client_udp_transceiver->EnableReadWrite();

  IoMessage msg;
  msg.seq_id = 0;
  msg.ip = addr.Ip();
  msg.port = addr.Port();
  msg.buffer = CreateBufferSlow(std::string(kDataSize, 1));
  client_udp_transceiver->Send(std::move(msg));

  while (server_received.load() == 0) {
    FiberSleepFor(std::chrono::milliseconds(1));
  }

  while (client_received.load() == 0) {
    FiberSleepFor(std::chrono::milliseconds(1));
  }

  ASSERT_EQ(server_received.load(), kDataSize);
  ASSERT_EQ(client_received.load(), kDataSize);

  IoMessage msg_err;
  msg.seq_id = 0;
  msg.ip = addr.Ip();
  msg.port = addr.Port();
  client_udp_transceiver->Send(std::move(msg));

  udp_transceiver->Stop();
  udp_transceiver->Join();
  client_udp_transceiver->Stop();
  client_udp_transceiver->Join();
}

}  // namespace trpc::testing

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
