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

#include "trpc/runtime/iomodel/reactor/default/uds_acceptor.h"

#include <memory>
#include <random>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/util/net_util.h"
#include "trpc/util/thread/latch.h"

namespace trpc {

namespace testing {

class UdsAcceptorTest : public ::testing::Test {
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

TEST_F(UdsAcceptorTest, UdsAcceptorOk) {
  int ret = 0;
  char path[] = "uds_accpetor_test.socket";
  UnixAddress addr = UnixAddress(path);

  Latch accept_latch(1);
  auto&& accept_handler = [&ret, &accept_latch](const AcceptConnectionInfo& connection_info) {
    ret = 1;
    accept_latch.count_down();
    return true;
  };

  sockaddr_un bad_socket;
  UnixAddress bad_addr = UnixAddress(&bad_socket);
  RefPtr<UdsAcceptor> fail_acceptor = MakeRefCounted<UdsAcceptor>(reactor_.get(), bad_addr);
  bool listen_fail = fail_acceptor->EnableListen();
  ASSERT_EQ(listen_fail, false);

  RefPtr<UdsAcceptor> acceptor = MakeRefCounted<UdsAcceptor>(reactor_.get(), addr);
  acceptor->SetAcceptHandleFunction(std::move(accept_handler));
  auto func = [](Socket& s) {};
  acceptor->SetAcceptSetSocketOptFunction(func);
  acceptor->EnableListen();

  std::unique_ptr<Socket> socket = std::make_unique<Socket>(Socket::CreateUnixSocket());

  int succ = socket->Connect(addr);

  ASSERT_EQ(succ, 0);

  accept_latch.wait();

  socket->Close();

  acceptor->DisableListen();

  ASSERT_EQ(ret, 1);
}

TEST_F(UdsAcceptorTest, UdsAcceptorFail) {
  Latch l(1);
  std::thread t1([this, &l]() {
    l.count_down();
    this->reactor_->Run();
  });

  l.wait();

  int ret = 0;
  char path[] = "uds_accpetor_test.socket";
  UnixAddress addr = UnixAddress(path);

  Latch accept_latch(1);
  auto&& accept_handler = [&ret, &accept_latch](const AcceptConnectionInfo& connection_info) {
    ret = 1;
    accept_latch.count_down();
    return false;
  };

  sockaddr_un bad_socket;
  UnixAddress bad_addr = UnixAddress(&bad_socket);
  RefPtr<UdsAcceptor> fail_acceptor = MakeRefCounted<UdsAcceptor>(reactor_.get(), bad_addr);
  bool listen_fail = fail_acceptor->EnableListen();
  EXPECT_EQ(listen_fail, false);

  RefPtr<UdsAcceptor> acceptor = MakeRefCounted<UdsAcceptor>(reactor_.get(), addr);
  acceptor->SetAcceptHandleFunction(std::move(accept_handler));
  auto func = [](Socket& s) {};
  acceptor->SetAcceptSetSocketOptFunction(func);
  acceptor->EnableListen();

  std::unique_ptr<Socket> socket = std::make_unique<Socket>(Socket::CreateUnixSocket());

  int succ = socket->Connect(addr);

  EXPECT_EQ(succ, 0);

  accept_latch.wait();

  socket->Close();

  acceptor->DisableListen();

  reactor_->Stop();

  t1.join();

  EXPECT_EQ(ret, 1);
}

}  // namespace testing

}  // namespace trpc
