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

#include "trpc/stream/trpc/trpc_server_stream_connection_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/server_codec.h"
#include "trpc/codec/server_codec_factory.h"
#include "trpc/codec/testing/server_codec_testing.h"
#include "trpc/stream/server_stream_handler_factory.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/transport/server/server_transport_def.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class FiberTrpcServerStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    conn_ = new MockTcpConnection();
    bind_info_ = new BindInfo();
    bind_info_->protocol = "MockServerCodec";
    conn_handler_ = std::make_unique<FiberTrpcServerStreamConnectionHandler>(conn_, nullptr, bind_info_);

    codec_ = std::make_shared<MockServerCodec>();
    stream_handler_ = CreateMockStreamHandler();


    ServerCodecFactory::GetInstance()->Register(codec_);
    ServerStreamHandlerFactory::GetInstance()->Register(
        "MockServerCodec", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ServerCodecFactory::GetInstance()->Clear();
    ServerStreamHandlerFactory::GetInstance()->Clear();
  }

 protected:
  std::unique_ptr<FiberTrpcServerStreamConnectionHandler> conn_handler_{nullptr};
  MockTcpConnection* conn_{nullptr};
  BindInfo* bind_info_{nullptr};
  std::shared_ptr<MockServerCodec> codec_{nullptr};
  MockStreamHandlerPtr stream_handler_{nullptr};
};

TEST_F(FiberTrpcServerStreamConnectionHandlerTest, InitForStreamingRpcTest) {
  EXPECT_CALL(*stream_handler_, Init()).WillOnce(::testing::Return(true));
  bind_info_->has_stream_rpc = true;
  conn_handler_->Init();
}

TEST_F(FiberTrpcServerStreamConnectionHandlerTest, InitForUnaryRpcTest) {
  EXPECT_CALL(*stream_handler_, Init()).Times(0);
  bind_info_->has_stream_rpc = false;
  conn_handler_->Init();
}

}  // namespace trpc::testing
