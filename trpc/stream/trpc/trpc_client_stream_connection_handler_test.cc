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

#include "trpc/stream/trpc/trpc_client_stream_connection_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/client_codec.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using namespace trpc::stream;

class MockClientCodec : public ClientCodec {
 public:
  std::string Name() const override { return "trpc"; }
  virtual ProtocolPtr CreateRequestPtr() override { return nullptr; }
  virtual ProtocolPtr CreateResponsePtr() override { return nullptr; }

  MOCK_METHOD(bool, Pick, (const std::any& message, std::any& metadata), (const, override));
};

class FiberTrpcClientStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    conn_ = std::make_unique<MockTcpConnection>();

    trans_info_ = std::make_unique<TransInfo>();
    trans_info_->protocol = "trpc";

    conn_handler_ = std::make_unique<FiberTrpcClientStreamConnectionHandler>(conn_.get(), trans_info_.get());
    conn_handler_->SetMsgHandleFunc([](const ConnectionPtr&, std::deque<std::any>&) { return true; });
    codec_ = std::make_shared<MockClientCodec>();
    stream_handler_ = CreateMockStreamHandler();

    ClientCodecFactory::GetInstance()->Register(codec_);
    ClientStreamHandlerFactory::GetInstance()->Register(
        "trpc", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ClientCodecFactory::GetInstance()->Clear();
    ClientStreamHandlerFactory::GetInstance()->Clear();
  }

 protected:
  std::unique_ptr<FiberTrpcClientStreamConnectionHandler> conn_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<TransInfo> trans_info_{nullptr};
  std::shared_ptr<MockClientCodec> codec_{nullptr};
  MockStreamHandlerPtr stream_handler_{nullptr};
};

TEST_F(FiberTrpcClientStreamConnectionHandlerTest, GetOrCreateStreamHandlerTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
    conn_handler_->Init();

    StreamHandlerPtr stream_handler = conn_handler_->GetOrCreateStreamHandler();

    StreamHandlerPtr check_stream_handler = conn_handler_->GetOrCreateStreamHandler();

    ASSERT_EQ(stream_handler.get(), check_stream_handler.get());
  });
}

TEST_F(FiberTrpcClientStreamConnectionHandlerTest, MessageHandleUnaryOnlyTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(0);

    bool execute_inner_msg_handle_function = false;
    conn_handler_->SetMsgHandleFunc([&](const ConnectionPtr&, std::deque<std::any>&) {
      execute_inner_msg_handle_function = true;
      return true;
    });

    ConnectionPtr conn = MakeRefCounted<Connection>();
    std::deque<std::any> msgs;
    ASSERT_TRUE(conn_handler_->HandleMessage(conn, msgs));

    ASSERT_TRUE(execute_inner_msg_handle_function);
  });
}

TEST_F(FiberTrpcClientStreamConnectionHandlerTest, MessageHandleStreamOnlyTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
    conn_handler_->Init();
    ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());

    EXPECT_CALL(*stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(1);

    TrpcProtocolMessageMetadata metadata{};
    metadata.enable_stream = true;
    metadata.stream_id = 100;
    metadata.stream_frame_type = 0;
    EXPECT_CALL(*codec_, Pick(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>(metadata), ::testing::Return(true)));

    ConnectionPtr conn = MakeRefCounted<Connection>();
    std::deque<std::any> msgs;
    msgs.push_back("stream");

    ASSERT_TRUE(conn_handler_->HandleMessage(conn, msgs));
  });
}

TEST_F(FiberTrpcClientStreamConnectionHandlerTest, MessageHandleStreamUnaryMixTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
    conn_handler_->Init();
    ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());

    EXPECT_CALL(*stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(1);

    TrpcProtocolMessageMetadata stream_metadata{};
    stream_metadata.enable_stream = true;
    TrpcProtocolMessageMetadata unary_metadata{};
    unary_metadata.enable_stream = false;
    EXPECT_CALL(*codec_, Pick(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>(stream_metadata), ::testing::Return(true)))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>(unary_metadata), ::testing::Return(true)));

    bool execute_inner_msg_handle_function = false;
    conn_handler_->SetMsgHandleFunc([&](const ConnectionPtr&, std::deque<std::any>&) {
      execute_inner_msg_handle_function = true;
      return true;
    });

    ConnectionPtr conn = MakeRefCounted<Connection>();
    std::deque<std::any> msgs;
    msgs.push_back("stream");
    msgs.push_back("unary");

    ASSERT_TRUE(conn_handler_->HandleMessage(conn, msgs));
    ASSERT_TRUE(execute_inner_msg_handle_function);
  });
}

TEST_F(FiberTrpcClientStreamConnectionHandlerTest, StopWhenHasStreamRpcTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
    conn_handler_->Init();
    ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());

    EXPECT_CALL(*stream_handler_, Stop()).Times(1);
    conn_handler_->Stop();
  });
}

TEST_F(FiberTrpcClientStreamConnectionHandlerTest, JoinWhenHasStreamRpcTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
    conn_handler_->Init();
    ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());

    EXPECT_CALL(*stream_handler_, Join()).Times(1);
    conn_handler_->Join();
  });
}

class FutureTrpcClientStreamConnComplexConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    rsp_dispatch_func_ = [](object_pool::LwUniquePtr<MsgTask>&& task) { return true; };
    rsp_decode_function_ = [this](std::any&& in, ProtocolPtr& out) {
      rsp_decode_num_++;
      return false;
    };
    conn_ = std::make_unique<MockTcpConnection>();

    trans_info_ = std::make_unique<TransInfo>();
    trans_info_->protocol = "trpc";
    trans_info_->rsp_decode_function = rsp_decode_function_;

    conn_group_options_ = std::make_unique<FutureConnectorGroupOptions>();
    conn_group_options_->trans_info = trans_info_.get();

    FutureConnectorOptions options;
    options.group_options = conn_group_options_.get();
    msg_timeout_handler_ = std::make_unique<FutureConnComplexMessageTimeoutHandler>(rsp_dispatch_func_);
    conn_handler_ =
        std::make_unique<FutureTrpcClientStreamConnComplexConnectionHandler>(options, *msg_timeout_handler_);

    codec_ = std::make_shared<MockClientCodec>();
    stream_handler_ = CreateMockStreamHandler();

    ClientCodecFactory::GetInstance()->Register(codec_);
    ClientStreamHandlerFactory::GetInstance()->Register(
        "trpc", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ClientCodecFactory::GetInstance()->Clear();
    ClientStreamHandlerFactory::GetInstance()->Clear();
  }

 protected:
  std::unique_ptr<FutureTrpcClientStreamConnComplexConnectionHandler> conn_handler_{nullptr};
  TransInfo::RspDispatchFunction rsp_dispatch_func_;
  TransInfo::RspDecodeFunction rsp_decode_function_;
  std::unique_ptr<FutureConnComplexMessageTimeoutHandler> msg_timeout_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<TransInfo> trans_info_{nullptr};
  std::unique_ptr<FutureConnectorGroupOptions> conn_group_options_{nullptr};
  std::shared_ptr<MockClientCodec> codec_{nullptr};
  MockStreamHandlerPtr stream_handler_{nullptr};

  std::size_t rsp_decode_num_{0};
};

TEST_F(FutureTrpcClientStreamConnComplexConnectionHandlerTest, GetOrCreateStreamHandlerTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();

  StreamHandlerPtr stream_handler = conn_handler_->GetOrCreateStreamHandler();

  StreamHandlerPtr check_stream_handler = conn_handler_->GetOrCreateStreamHandler();

  ASSERT_EQ(stream_handler.get(), check_stream_handler.get());
}

//TEST_F(FutureTrpcClientStreamConnComplexConnectionHandlerTest, MessageHandleUnaryOnlyTest) {
//  EXPECT_CALL(*stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(0);
//
//  ConnectionPtr conn = MakeRefCounted<Connection>();
//  std::deque<std::any> msgs;
//  // FIXME: it may crash.
//  ASSERT_TRUE(conn_handler_->HandleMessage(conn, msgs));
//}

TEST_F(FutureTrpcClientStreamConnComplexConnectionHandlerTest, MessageHandleStreamOnlyTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());

  EXPECT_CALL(*stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(1);

  TrpcProtocolMessageMetadata metadata{};
  metadata.enable_stream = true;
  metadata.stream_id = 100;
  metadata.stream_frame_type = 0;
  EXPECT_CALL(*codec_, Pick(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>(metadata), ::testing::Return(true)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  std::deque<std::any> msgs;
  msgs.push_back("stream");

  ASSERT_TRUE(conn_handler_->HandleMessage(conn, msgs));
}

//TEST_F(FutureTrpcClientStreamConnComplexConnectionHandlerTest, MessageHandleStreamUnaryMixTest) {
//  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
//  conn_handler_->Init();
//  ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());
//
//  EXPECT_CALL(*stream_handler_, PushMessage_rvr(::testing::_, ::testing::_)).Times(1);
//
//  TrpcProtocolMessageMetadata stream_metadata{};
//  stream_metadata.enable_stream = true;
//  TrpcProtocolMessageMetadata unary_metadata{};
//  unary_metadata.enable_stream = false;
//  EXPECT_CALL(*codec_, Pick(::testing::_, ::testing::_))
//      .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>(stream_metadata), ::testing::Return(true)))
//      .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>(unary_metadata), ::testing::Return(true)));
//
//  ConnectionPtr conn = MakeRefCounted<Connection>();
//  std::deque<std::any> msgs;
//  msgs.push_back("stream");
//  msgs.push_back("unary");
//  // FIXME: it may crash.
//  ASSERT_TRUE(conn_handler_->HandleMessage(conn, msgs));
//}

TEST_F(FutureTrpcClientStreamConnComplexConnectionHandlerTest, StopWhenHasStreamRpcTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());

  EXPECT_CALL(*stream_handler_, Stop()).Times(1);
  conn_handler_->Stop();
}

TEST_F(FutureTrpcClientStreamConnComplexConnectionHandlerTest, JoinWhenHasStreamRpcTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  ASSERT_TRUE(conn_handler_->GetOrCreateStreamHandler());

  EXPECT_CALL(*stream_handler_, Join()).Times(1);
  conn_handler_->Join();
}

}  // namespace trpc::testing
