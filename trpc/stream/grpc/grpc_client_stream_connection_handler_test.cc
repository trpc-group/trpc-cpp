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

#include "trpc/stream/grpc/grpc_client_stream_connection_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/client_codec.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/grpc/grpc_io_handler.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockClientCodec : public ClientCodec {
 public:
  std::string Name() const override { return "grpc"; }
  virtual ProtocolPtr CreateRequestPtr() override { return nullptr; }
  virtual ProtocolPtr CreateResponsePtr() override { return nullptr; }
  MOCK_METHOD(bool, Pick, (const std::any& message, std::any& metadata), (const, override));
};

class GrpcMockStreamHandler : public MockStreamHandler {
 public:
  StreamOptions* GetMutableStreamOptions() override { return &options_; }

 private:
  StreamOptions options_;
};
using GrpcMockStreamHandlerPtr = RefPtr<GrpcMockStreamHandler>;

class FiberGrpcClientStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    conn_ = std::make_unique<MockTcpConnection>();
    conn_->SetIoHandler(std::make_unique<GrpcIoHandler>(conn_.get()));

    trans_info_ = std::make_unique<TransInfo>();
    trans_info_->protocol = "grpc";

    conn_handler_ = std::make_unique<FiberGrpcClientStreamConnectionHandler>(conn_.get(), trans_info_.get());
    conn_handler_->SetMsgHandleFunc([](const ConnectionPtr&, std::deque<std::any>&) { return true; });

    codec_ = std::make_shared<MockClientCodec>();
    stream_handler_ = MakeRefCounted<GrpcMockStreamHandler>();

    ClientCodecFactory::GetInstance()->Register(codec_);
    ClientStreamHandlerFactory::GetInstance()->Register(
        "grpc", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ClientCodecFactory::GetInstance()->Clear();
    ClientStreamHandlerFactory::GetInstance()->Clear();
  }

  void MockInit() {}

 protected:
  std::unique_ptr<FiberGrpcClientStreamConnectionHandler> conn_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<TransInfo> trans_info_{nullptr};
  std::shared_ptr<MockClientCodec> codec_{nullptr};
  GrpcMockStreamHandlerPtr stream_handler_{nullptr};
};

TEST_F(FiberGrpcClientStreamConnectionHandlerTest, InitOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();
}

// TEST_F(FiberGrpcClientStreamConnectionHandlerTest, GetOrCreateStreamHandlerTest) {
//   RunAsFiber([&]() {
//     EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
//     conn_handler_->Init();
//
//     StreamHandlerPtr stream_handler = conn_handler_->GetOrCreateStreamHandler();
//     StreamHandlerPtr check_stream_handler = conn_handler_->GetOrCreateStreamHandler();
//     ASSERT_EQ(stream_handler.get(), check_stream_handler.get());
//   });
// }

TEST_F(FiberGrpcClientStreamConnectionHandlerTest, HandleMessageUnaryOnlyTest) {
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
}

TEST_F(FiberGrpcClientStreamConnectionHandlerTest, CheckMessageOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  std::deque<std::any> parse_out;
  parse_out.push_back(1);
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_FULL);
}

TEST_F(FiberGrpcClientStreamConnectionHandlerTest, CheckMessageFailedTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  std::deque<std::any> parse_out;
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_LESS);

  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_ERR);
}

TEST_F(FiberGrpcClientStreamConnectionHandlerTest, EncodeStreamMessageOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  EXPECT_CALL(*stream_handler_, EncodeTransportMessage(::testing::_))
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(1));
  IoMessage msg;
  ASSERT_TRUE(conn_handler_->EncodeStreamMessage(&msg));
  ASSERT_FALSE(conn_handler_->EncodeStreamMessage(&msg));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FutureGrpcClientStreamConnPoolConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    rsp_dispatch_func_ = [](object_pool::LwUniquePtr<MsgTask>&& task) { return true; };

    conn_ = std::make_unique<MockTcpConnection>();
    conn_->SetIoHandler(std::make_unique<GrpcIoHandler>(conn_.get()));

    trans_info_ = std::make_unique<TransInfo>();
    trans_info_->protocol = "grpc";

    conn_group_options_ = std::make_unique<FutureConnectorGroupOptions>();
    conn_group_options_->trans_info = trans_info_.get();

    FutureConnectorOptions options;
    options.group_options = conn_group_options_.get();
    msg_timeout_handler_ =
        std::make_unique<FutureConnPoolMessageTimeoutHandler>(0, rsp_dispatch_func_, shared_send_queue_);
    conn_handler_ = std::make_unique<FutureGrpcClientStreamConnPoolConnectionHandler>(options, *msg_timeout_handler_);
    conn_handler_->SetConnection(conn_.get());

    codec_ = std::make_shared<MockClientCodec>();
    stream_handler_ = MakeRefCounted<GrpcMockStreamHandler>();

    ClientCodecFactory::GetInstance()->Register(codec_);
    ClientStreamHandlerFactory::GetInstance()->Register(
        "grpc", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ClientCodecFactory::GetInstance()->Clear();
    ClientStreamHandlerFactory::GetInstance()->Clear();
  }

  void MockInit() {}

 protected:
  std::unique_ptr<FutureGrpcClientStreamConnPoolConnectionHandler> conn_handler_{nullptr};
  internal::SharedSendQueue shared_send_queue_{1};
  TransInfo::RspDispatchFunction rsp_dispatch_func_;
  std::unique_ptr<FutureConnPoolMessageTimeoutHandler> msg_timeout_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<TransInfo> trans_info_{nullptr};
  std::unique_ptr<FutureConnectorGroupOptions> conn_group_options_{nullptr};
  std::shared_ptr<MockClientCodec> codec_{nullptr};
  MockStreamHandlerPtr stream_handler_{nullptr};
};

TEST_F(FutureGrpcClientStreamConnPoolConnectionHandlerTest, InitOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();
}

TEST_F(FutureGrpcClientStreamConnPoolConnectionHandlerTest, CheckMessageOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  std::deque<std::any> parse_out;
  parse_out.push_back(1);
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_FULL);
}

TEST_F(FutureGrpcClientStreamConnPoolConnectionHandlerTest, CheckMessageFailedTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  std::deque<std::any> parse_out;
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_LESS);

  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_ERR);
}

TEST_F(FutureGrpcClientStreamConnPoolConnectionHandlerTest, EncodeStreamMessageOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  EXPECT_CALL(*stream_handler_, EncodeTransportMessage(::testing::_))
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(1));
  IoMessage msg;
  ASSERT_TRUE(conn_handler_->EncodeStreamMessage(&msg));
  ASSERT_FALSE(conn_handler_->EncodeStreamMessage(&msg));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FutureClientGrpcStreamConnComplexConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    rsp_dispatch_func_ = [](object_pool::LwUniquePtr<MsgTask>&& task) { return true; };
    conn_ = std::make_unique<MockTcpConnection>();
    conn_->SetIoHandler(std::make_unique<GrpcIoHandler>(conn_.get()));

    trans_info_ = std::make_unique<TransInfo>();
    trans_info_->protocol = "grpc";

    conn_group_options_ = std::make_unique<FutureConnectorGroupOptions>();
    conn_group_options_->trans_info = trans_info_.get();

    FutureConnectorOptions options;
    options.group_options = conn_group_options_.get();
    msg_timeout_handler_ = std::make_unique<FutureConnComplexMessageTimeoutHandler>(rsp_dispatch_func_);
    conn_handler_ =
        std::make_unique<FutureGrpcClientStreamConnComplexConnectionHandler>(options, *msg_timeout_handler_);
    conn_handler_->SetConnection(conn_.get());

    codec_ = std::make_shared<MockClientCodec>();
    stream_handler_ = MakeRefCounted<GrpcMockStreamHandler>();

    ClientCodecFactory::GetInstance()->Register(codec_);
    ClientStreamHandlerFactory::GetInstance()->Register(
        "grpc", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ClientCodecFactory::GetInstance()->Clear();
    ClientStreamHandlerFactory::GetInstance()->Clear();
  }

  void MockInit() {}

 protected:
  std::unique_ptr<FutureGrpcClientStreamConnComplexConnectionHandler> conn_handler_{nullptr};
  TransInfo::RspDispatchFunction rsp_dispatch_func_;
  std::unique_ptr<FutureConnComplexMessageTimeoutHandler> msg_timeout_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<TransInfo> trans_info_{nullptr};
  std::unique_ptr<FutureConnectorGroupOptions> conn_group_options_{nullptr};
  std::shared_ptr<MockClientCodec> codec_{nullptr};
  MockStreamHandlerPtr stream_handler_{nullptr};
};

TEST_F(FutureClientGrpcStreamConnComplexConnectionHandlerTest, InitOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();
}

TEST_F(FutureClientGrpcStreamConnComplexConnectionHandlerTest, CheckMessageOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  std::deque<std::any> parse_out;
  parse_out.push_back(1);
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_FULL);
}

TEST_F(FutureClientGrpcStreamConnComplexConnectionHandlerTest, CheckMessageFailedTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  std::deque<std::any> parse_out;
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_LESS);

  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_ERR);
}

TEST_F(FutureClientGrpcStreamConnComplexConnectionHandlerTest, EncodeStreamMessageOkTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  conn_handler_->Init();
  conn_handler_->DoHandshake();

  EXPECT_CALL(*stream_handler_, EncodeTransportMessage(::testing::_))
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(1));
  IoMessage msg;
  ASSERT_TRUE(conn_handler_->EncodeStreamMessage(&msg));
  ASSERT_FALSE(conn_handler_->EncodeStreamMessage(&msg));
}

}  // namespace trpc::testing
