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

#include "trpc/runtime/iomodel/reactor/default/tcp_acceptor.h"

#include <memory>
#include <random>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/util/net_util.h"
#include "trpc/util/thread/latch.h"

namespace trpc::testing {

class TcpAcceptorTest : public ::testing::Test {
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

TEST_F(TcpAcceptorTest, All) {
  int ret = 0;

  Latch accept_latch(1);
  auto&& accept_handler = [&ret, &accept_latch](const AcceptConnectionInfo& connection_info) {
    ret = 1;
    accept_latch.count_down();
    return true;
  };

  // invalid ip 1.1.1.1，socket bind will failed
  NetworkAddress bad_addr = NetworkAddress("1.1.1.1", 10000, NetworkAddress::IpType::kIpV4);
  RefPtr<TcpAcceptor> fail_acceptor = MakeRefCounted<TcpAcceptor>(reactor_.get(), bad_addr);
  bool listen_fail = fail_acceptor->EnableListen();
  ASSERT_EQ(listen_fail, false);

  NetworkAddress addr = NetworkAddress(trpc::util::GenRandomAvailablePort(), false, NetworkAddress::IpType::kIpV4);

  RefPtr<TcpAcceptor> acceptor = MakeRefCounted<TcpAcceptor>(reactor_.get(), addr);
  acceptor->SetAcceptHandleFunction(std::move(accept_handler));
  acceptor->SetAcceptSetSocketOptFunction([](Socket& socket) {});

  Latch enable(1);

  Reactor::Task task1 = [&enable, &acceptor]() {
    acceptor->EnableListen();

    enable.count_down();
  };

  reactor_->SubmitTask(std::move(task1));

  enable.wait();

  std::unique_ptr<Socket> socket = std::make_unique<Socket>(Socket::CreateTcpSocket(false));

  int succ = socket->Connect(addr);

  ASSERT_EQ(succ, 0);

  accept_latch.wait();

  socket->Close();

  Latch disabled(1);

  Reactor::Task task2 = [&disabled, &acceptor]() {
    acceptor->DisableListen();

    disabled.count_down();
  };

  reactor_->SubmitTask(std::move(task2));

  disabled.wait();

  ASSERT_EQ(ret, 1);
}

}  // namespace trpc::testing
