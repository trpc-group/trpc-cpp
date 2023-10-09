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

#include "trpc/stream/http/async/client/stream_connection_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using namespace trpc::stream;

static internal::SharedSendQueue send_queue(64);

class HttpClientAsyncStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_conn_ = MakeRefCounted<MockTcpConnection>();
    timeout_handler_ = std::make_unique<FutureConnPoolMessageTimeoutHandler>(
        1, [](object_pool::LwUniquePtr<MsgTask>&& task) { return true; }, send_queue);

    trans_info_ = std::make_unique<TransInfo>();
    trans_info_->protocol = "http";

    conn_group_options_ = std::make_unique<FutureConnectorGroupOptions>();
    conn_group_options_->trans_info = trans_info_.get();

    connector_options_.group_options = conn_group_options_.get();
    conn_handler_ = std::make_unique<HttpClientAsyncStreamConnectionHandler>(connector_options_, *timeout_handler_);
    conn_handler_->SetConnection(mock_conn_.get());

    mock_stream_handler_ = CreateMockStreamHandler();
    ClientStreamHandlerFactory::GetInstance()->Register(
        "http", [&](StreamOptions&& options) -> StreamHandlerPtr { return mock_stream_handler_; });
  }

  void TearDown() override { ClientStreamHandlerFactory::GetInstance()->Clear(); }

  void InitMock() {
    EXPECT_CALL(*mock_stream_handler_, Init()).WillOnce(::testing::Return(true));
  }

 protected:
  std::unique_ptr<HttpClientAsyncStreamConnectionHandler> conn_handler_{nullptr};
  RefPtr<MockTcpConnection> mock_conn_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
  std::unique_ptr<TransInfo> trans_info_{nullptr};

 private:
  FutureConnectorOptions connector_options_;
  std::unique_ptr<FutureConnectorGroupOptions> conn_group_options_{nullptr};
  std::unique_ptr<FutureConnPoolMessageTimeoutHandler> timeout_handler_{nullptr};
};

TEST_F(HttpClientAsyncStreamConnectionHandlerTest, TestGetOrCreateStreamHandler) {
  InitMock();
  StreamHandlerPtr stream_handler = conn_handler_->GetOrCreateStreamHandler();
  ASSERT_TRUE(stream_handler != nullptr);
  ASSERT_TRUE(stream_handler.Get() == conn_handler_->GetOrCreateStreamHandler().Get());
}

TEST_F(HttpClientAsyncStreamConnectionHandlerTest, TestUnaryCheckMessage) {
  trans_info_->is_stream_proxy = false;
  bool invoke_http_unary = false;
  trans_info_->checker_function = [&](const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) {
    invoke_http_unary = true;
    return 233;
  };
  NoncontiguousBuffer buf;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(mock_conn_, buf, out), 233);
  ASSERT_TRUE(invoke_http_unary);
}

TEST_F(HttpClientAsyncStreamConnectionHandlerTest, TestStreamCheckMessage) {
  InitMock();
  trans_info_->is_stream_proxy = true;
  ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler() != nullptr);

  EXPECT_CALL(*mock_stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
  NoncontiguousBuffer buf;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(mock_conn_, buf, out), PacketChecker::PACKET_FULL);

  EXPECT_CALL(*mock_stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(-2));
  ASSERT_EQ(conn_handler_->CheckMessage(mock_conn_, buf, out), PacketChecker::PACKET_LESS);

  EXPECT_CALL(*mock_stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_EQ(conn_handler_->CheckMessage(mock_conn_, buf, out), PacketChecker::PACKET_ERR);
}

TEST_F(HttpClientAsyncStreamConnectionHandlerTest, TestUnaryHandleMessage) {
  trans_info_->is_stream_proxy = false;
  bool invoke_http_unary = false;
  trans_info_->rsp_decode_function = [&](std::any&& in, ProtocolPtr& out) {
    invoke_http_unary = true;
    return false;
  };
  std::deque<std::any> out;
  out.push_back(1);

  ASSERT_FALSE(conn_handler_->HandleMessage(mock_conn_, out));
  ASSERT_TRUE(invoke_http_unary);
}

TEST_F(HttpClientAsyncStreamConnectionHandlerTest, TestStreamHandleMessage) {
  InitMock();
  trans_info_->is_stream_proxy = true;
  ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler() != nullptr);
  EXPECT_CALL(*mock_stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(2);
  std::deque<std::any> out;
  out.push_back(1);
  out.push_back(2);
  ASSERT_TRUE(conn_handler_->HandleMessage(mock_conn_, out));
}

}  // namespace trpc::testing
