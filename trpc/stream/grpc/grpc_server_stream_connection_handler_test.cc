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

#include "trpc/stream/grpc/grpc_server_stream_connection_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/server_codec_factory.h"
#include "trpc/codec/testing/server_codec_testing.h"
#include "trpc/stream/grpc/grpc_io_handler.h"
#include "trpc/stream/server_stream_handler_factory.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/transport/server/server_transport_def.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class FiberGrpcServerStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    conn_ = std::make_unique<MockTcpConnection>();
    conn_->SetIoHandler(std::make_unique<GrpcIoHandler>(conn_.get()));

    bind_info_ = std::make_unique<BindInfo>();
    bind_info_->protocol = "MockServerCodec";
    bind_info_->has_stream_rpc = false;
    bind_info_->check_stream_rpc_function = [](const std::string& path) { return true; };

    conn_handler_ = std::make_unique<FiberGrpcServerStreamConnectionHandler>(conn_.get(), nullptr, bind_info_.get());

    // MockServerCodec.
    codec_ = std::make_shared<MockServerCodec>();
    ServerCodecFactory::GetInstance()->Register(codec_);

    // MockStreamHandler.
    stream_handler_ = MakeRefCounted<MockStreamHandler>();
    ServerStreamHandlerFactory::GetInstance()->Register(
        "MockServerCodec", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ServerCodecFactory::GetInstance()->Clear();
    ServerStreamHandlerFactory::GetInstance()->Clear();
  }

  void MockInit() {
    EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
    EXPECT_CALL(*stream_handler_, GetMutableStreamOptions()).WillRepeatedly(::testing::Return(&stream_options_));
  }

 protected:
  std::unique_ptr<FiberGrpcServerStreamConnectionHandler> conn_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<BindInfo> bind_info_{nullptr};
  std::shared_ptr<MockServerCodec> codec_{nullptr};
  MockStreamHandlerPtr stream_handler_{nullptr};
  stream::StreamOptions stream_options_;
};

TEST_F(FiberGrpcServerStreamConnectionHandlerTest, InitForStreamingRpc) {
  MockInit();
  bind_info_->has_stream_rpc = false;
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());
}

TEST_F(FiberGrpcServerStreamConnectionHandlerTest, InitForUnaryRpc) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());
}

TEST_F(FiberGrpcServerStreamConnectionHandlerTest, CheckMessageOk) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());

  std::deque<std::any> parse_out;
  parse_out.push_back(1);
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_FULL);
}

TEST_F(FiberGrpcServerStreamConnectionHandlerTest, CheckMessageFailed) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());

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

TEST_F(FiberGrpcServerStreamConnectionHandlerTest, EncodeStreamMessageOkTest) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());

  EXPECT_CALL(*stream_handler_, EncodeTransportMessage(::testing::_))
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(1));
  IoMessage msg;
  ASSERT_TRUE(conn_handler_->EncodeStreamMessage(&msg));
  ASSERT_FALSE(conn_handler_->EncodeStreamMessage(&msg));
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////

class ServerGrpcStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    conn_ = std::make_unique<MockTcpConnection>();
    conn_->SetIoHandler(std::make_unique<GrpcIoHandler>(conn_.get()));

    bind_info_ = std::make_unique<BindInfo>();
    bind_info_->protocol = "MockServerCodec";
    bind_info_->has_stream_rpc = false;
    bind_info_->check_stream_rpc_function = [](const std::string& path) { return true; };

    conn_handler_ = std::make_unique<DefaultGrpcServerStreamConnectionHandler>(conn_.get(), nullptr, bind_info_.get());

    // MockServerCodec.
    codec_ = std::make_shared<MockServerCodec>();
    ServerCodecFactory::GetInstance()->Register(codec_);

    // MockStreamHandler.
    stream_handler_ = MakeRefCounted<MockStreamHandler>();
    ServerStreamHandlerFactory::GetInstance()->Register(
        "MockServerCodec", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ServerCodecFactory::GetInstance()->Clear();
    ServerStreamHandlerFactory::GetInstance()->Clear();
  }

  void MockInit() {
    EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
    EXPECT_CALL(*stream_handler_, GetMutableStreamOptions()).WillRepeatedly(::testing::Return(&stream_options_));
  }

 protected:
  std::unique_ptr<DefaultGrpcServerStreamConnectionHandler> conn_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<BindInfo> bind_info_{nullptr};
  std::shared_ptr<MockServerCodec> codec_{nullptr};
  MockStreamHandlerPtr stream_handler_{nullptr};
  stream::StreamOptions stream_options_;
};

TEST_F(ServerGrpcStreamConnectionHandlerTest, InitForStreamingRpc) {
  MockInit();
  bind_info_->has_stream_rpc = false;
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());
}

TEST_F(ServerGrpcStreamConnectionHandlerTest, InitForUnaryRpc) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());
}

TEST_F(ServerGrpcStreamConnectionHandlerTest, CheckMessageOk) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());

  std::deque<std::any> parse_out;
  parse_out.push_back(1);
  EXPECT_CALL(*stream_handler_, ParseMessage(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(parse_out), ::testing::Return(0)));

  ConnectionPtr conn = MakeRefCounted<Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(conn_handler_->CheckMessage(conn, in, out), PacketChecker::PACKET_FULL);
}

TEST_F(ServerGrpcStreamConnectionHandlerTest, CheckMessageCheckFailed) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());

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

TEST_F(ServerGrpcStreamConnectionHandlerTest, EncodeStreamMessageOk) {
  MockInit();
  conn_handler_->Init();
  ASSERT_EQ(0, conn_handler_->DoHandshake());

  EXPECT_CALL(*stream_handler_, EncodeTransportMessage(::testing::_))
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(1));
  IoMessage msg;
  ASSERT_TRUE(conn_handler_->EncodeStreamMessage(&msg));
  ASSERT_FALSE(conn_handler_->EncodeStreamMessage(&msg));
}

}  // namespace trpc::testing
