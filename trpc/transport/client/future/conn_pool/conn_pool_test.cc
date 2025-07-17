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

#include "trpc/transport/client/future/conn_pool/conn_pool.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/util/net_util.h"

namespace trpc::testing {

class TestFutureConnector : public FutureConnector {
 public:
  TestFutureConnector(FutureConnectorOptions&& options) : FutureConnector(std::move(options)) {}

  int SendReqMsg(CTransportReqMsg* req_msg) override { return 0; }

  int SendOnly(CTransportReqMsg* req_msg) override { return 0; }

  ConnectionState GetConnectionState() override { return ConnectionState::kConnected; }
};

using TestConnPool = ConnPool<TestFutureConnector>;

class ConnPoolFixture : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    trpc::ReactorImpl::Options reactor_options;
    reactor_options.id = 0;
    reactor_ = std::make_unique<ReactorImpl>(reactor_options);
    reactor_->Initialize();

    trans_info_.max_conn_num = 2;
    group_options_.reactor = reactor_.get();
    group_options_.trans_info = &trans_info_;
    group_options_.peer_addr =
        NetworkAddress("127.0.0.1", trpc::util::GenRandomAvailablePort(), NetworkAddress::IpType::kIpV4);
  }

  static void TearDownTestCase() { reactor_->Destroy(); }

  void SetUp() override { conn_pool_ = std::make_unique<TestConnPool>(group_options_); }

  void TearDown() override {
    if (conn_pool_) {
      conn_pool_.reset();
    }
  }

 protected:
  static FutureConnectorGroupOptions group_options_;
  static TransInfo trans_info_;
  static std::unique_ptr<ReactorImpl> reactor_;
  std::unique_ptr<TestConnPool> conn_pool_;
};

FutureConnectorGroupOptions ConnPoolFixture::group_options_;
TransInfo ConnPoolFixture::trans_info_;
std::unique_ptr<ReactorImpl> ConnPoolFixture::reactor_ = nullptr;

TEST_F(ConnPoolFixture, FlowTest) {
  EXPECT_TRUE(conn_pool_->GetConnectors().size() >= trans_info_.max_conn_num);
  EXPECT_EQ(conn_pool_->SizeOfFree(), trans_info_.max_conn_num);
  EXPECT_TRUE(conn_pool_->IsEmpty());

  // Add a connection to the connection pool.
  uint64_t conn_id1 = conn_pool_->GenAvailConnectorId();
  EXPECT_EQ(conn_id1, 1);
  auto mock_conn1 = std::make_unique<TestFutureConnector>(FutureConnectorOptions{});
  EXPECT_TRUE(conn_pool_->AddConnector(conn_id1, std::move(mock_conn1)));
  EXPECT_TRUE(conn_pool_->GetConnector(conn_id1) != nullptr);

  // Add a connection to the connection pool again.
  uint64_t conn_id2 = conn_pool_->GenAvailConnectorId();
  EXPECT_EQ(conn_id2, 0);
  auto mock_conn2 = std::make_unique<TestFutureConnector>(FutureConnectorOptions{});
  EXPECT_TRUE(conn_pool_->AddConnector(conn_id2, std::move(mock_conn2)));
  EXPECT_TRUE(conn_pool_->GetConnector(conn_id2) != nullptr);

  // Check if the connection pool is full.
  EXPECT_EQ(conn_pool_->SizeOfFree(), 0);
  uint64_t conn_id = conn_pool_->GenAvailConnectorId();
  EXPECT_TRUE(conn_id == TestConnPool::kInvalidConnId);

  // Recycle connection
  conn_pool_->RecycleConnectorId(conn_id1);
  EXPECT_EQ(conn_pool_->SizeOfFree(), 1);
  conn_id = conn_pool_->GenAvailConnectorId();
  EXPECT_EQ(conn_id, conn_id1);
  EXPECT_TRUE(conn_pool_->GetConnector(conn_id) != nullptr);

  conn_pool_->DelConnector(conn_id1);
  conn_pool_->DelConnector(conn_id2);
  EXPECT_EQ(conn_pool_->SizeOfFree(), 2);
}

TEST_F(ConnPoolFixture, PendingQueue) {
  CTransportReqMsg msg;
  msg.extend_info = trpc::object_pool::MakeLwShared<trpc::ClientExtendInfo>();
  msg.context = MakeRefCounted<ClientContext>();
  msg.context->SetRequestId(1);
  msg.context->SetTimeout(100);
  conn_pool_->PushToPendingQueue(&msg);

  CTransportReqMsg* req_msg = conn_pool_->PopFromPendingQueue();
  EXPECT_TRUE(req_msg != nullptr);

  // Timeout test for requests in the pending queue.
  trans_info_.rsp_dispatch_function = [](object_pool::LwUniquePtr<MsgTask>&& task) {
    task->handler();
    return true;
  };

  conn_pool_->PushToPendingQueue(&msg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  auto fut = msg.extend_info->promise.GetFuture();
  conn_pool_->HandlePendingQueueTimeout();
  EXPECT_EQ(fut.IsFailed(), true);
}

}  // namespace trpc::testing
