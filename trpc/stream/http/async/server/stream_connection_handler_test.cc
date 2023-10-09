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

#include "trpc/stream/http/async/server/stream_connection_handler.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/stream/server_stream_handler_factory.h"
#include "trpc/stream/stream.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using namespace trpc::stream;

class MockDefaultHttpServerStreamConnectionHandler : public DefaultHttpServerStreamConnectionHandler {
 public:
  MockDefaultHttpServerStreamConnectionHandler(Connection* conn, BindAdapter* bind_adapter, BindInfo* bind_info)
      : DefaultHttpServerStreamConnectionHandler(conn, bind_adapter, bind_info) {}

  StreamHandlerPtr GetStreamHandler() { return stream_handler_; }
};

class HttpServerAsyncStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    codec::Init();

    mock_conn_ = MakeRefCounted<MockTcpConnection>();
    bind_info_ = new BindInfo();
    bind_info_->protocol = "http";
    mock_conn_handler_ =
        std::make_unique<MockDefaultHttpServerStreamConnectionHandler>(mock_conn_.get(), nullptr, bind_info_);

    mock_stream_handler_ = CreateMockStreamHandler();
    ServerStreamHandlerFactory::GetInstance()->Register(
        "http", [&](StreamOptions&& options) -> StreamHandlerPtr { return mock_stream_handler_; });
  }

  void TearDown() override {
    ServerStreamHandlerFactory::GetInstance()->Clear();
    codec::Destroy();
  }

 protected:
  RefPtr<MockTcpConnection> mock_conn_{nullptr};
  std::unique_ptr<MockDefaultHttpServerStreamConnectionHandler> mock_conn_handler_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
  BindInfo* bind_info_{nullptr};
};

TEST_F(HttpServerAsyncStreamConnectionHandlerTest, Init) {
  ASSERT_TRUE(mock_conn_handler_->GetStreamHandler() == nullptr);
  EXPECT_CALL(*mock_stream_handler_, Init()).WillOnce(::testing::Return(true));
  mock_conn_handler_->Init();
  ASSERT_TRUE(mock_conn_handler_->GetStreamHandler() != nullptr);
}

TEST_F(HttpServerAsyncStreamConnectionHandlerTest, CheckUnaryMessage) {
  bind_info_->has_stream_rpc = false;
  bool execute_fn = false;
  bind_info_->checker_function = [&](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    execute_fn = true;
    return 233;
  };
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(mock_conn_handler_->CheckMessage(mock_conn_, in, out), 233);
  ASSERT_TRUE(execute_fn);
}

TEST_F(HttpServerAsyncStreamConnectionHandlerTest, HandleUnaryMessage) {
  bind_info_->has_stream_rpc = false;
  bool execute_fn = false;
  bind_info_->msg_handle_function = [&](const ConnectionPtr& conn, std::deque<std::any>& out) {
    execute_fn = true;
    return true;
  };
  std::deque<std::any> out;
  ASSERT_TRUE(mock_conn_handler_->HandleMessage(mock_conn_, out));
  ASSERT_TRUE(execute_fn);
}

TEST_F(HttpServerAsyncStreamConnectionHandlerTest, CheckStreamMessage) {
  bind_info_->has_stream_rpc = true;
  EXPECT_CALL(*mock_stream_handler_, Init()).WillOnce(::testing::Return(true));
  mock_conn_handler_->Init();
  ASSERT_TRUE(mock_conn_handler_->GetStreamHandler() != nullptr);
  // parses ok
  EXPECT_CALL(*mock_stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
  NoncontiguousBuffer buf;
  std::deque<std::any> out;
  ASSERT_EQ(mock_conn_handler_->CheckMessage(mock_conn_, buf, out), PacketChecker::PACKET_FULL);
  // incomplete message
  EXPECT_CALL(*mock_stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(-2));
  ASSERT_EQ(mock_conn_handler_->CheckMessage(mock_conn_, buf, out), PacketChecker::PACKET_LESS);
  // wrong message
  EXPECT_CALL(*mock_stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_EQ(mock_conn_handler_->CheckMessage(mock_conn_, buf, out), PacketChecker::PACKET_ERR);
}

TEST_F(HttpServerAsyncStreamConnectionHandlerTest, HandleStreamMessage) {
  bind_info_->has_stream_rpc = true;
  EXPECT_CALL(*mock_stream_handler_, Init()).WillOnce(::testing::Return(true));
  mock_conn_handler_->Init();
  EXPECT_CALL(*mock_stream_handler_, IsNewStream(::testing::_, ::testing::_)).WillOnce(::testing::Return(true));
  EXPECT_CALL(*mock_stream_handler_, CreateStream_rvr(::testing::_))
      .WillOnce(::testing::Return(CreateErrorStreamProvider(Status{233, 0, "test233"})));
  EXPECT_CALL(*mock_stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(1);
  std::deque<std::any> out;
  out.push_back(1);
  ASSERT_TRUE(mock_conn_handler_->HandleMessage(mock_conn_, out));
}

}  // namespace trpc::testing
