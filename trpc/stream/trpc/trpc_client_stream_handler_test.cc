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

#include "trpc/stream/trpc/trpc_client_stream_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/trpc/testing/trpc_stream_testing.h"

namespace trpc::testing {

using namespace trpc::stream;

class TrpcClientStreamHandlerTest : public ::testing::Test {
 public:
  using TrpcClientStreamHandlerPtr = RefPtr<TrpcClientStreamHandler>;

  TrpcClientStreamHandlerPtr CreateTrpcClientStreamHandler() {
    StreamOptions opt;
    opt.server_mode = false;
    opt.fiber_mode = true;
    return MakeRefCounted<TrpcClientStreamHandler>(std::move(opt));
  }

  static void SetUpTestSuite() { trpc::codec::Init(); }

  static void TearDownTestSuite() { trpc::codec::Destroy(); }

  void SetUp() override { stream_handler_ = CreateTrpcClientStreamHandler(); }

  void TearDown() override {}

  void MockSend(int rc) {
    stream_handler_->GetMutableStreamOptions()->send = [rc](IoMessage&& msg) { return rc; };
  }

 protected:
  TrpcClientStreamHandlerPtr stream_handler_;
};

TEST_F(TrpcClientStreamHandlerTest, CreateStreamOk) {
  RunAsFiber([&]() {
    MockSend(0);

    stream::StreamOptions options;
    options.stream_id = 100;
    options.stream_handler = stream_handler_;
    options.server_mode = false;
    options.fiber_mode = true;
    options.context.context = MakeRefCounted<ClientContext>();
    auto stream = stream_handler_->CreateStream(std::move(options));
    EXPECT_TRUE(stream);

    // Terminate the stream coroutine by pushing RESET.
    stream_handler_->Stop();
    stream_handler_->Join();
  });
}

TEST_F(TrpcClientStreamHandlerTest, SendMessageOk) {
  RunAsFiber([&]() {
    MockSend(0);
    ASSERT_EQ(stream_handler_->SendMessage(stream_handler_->GetMutableStreamOptions()->context.context,
                                           NoncontiguousBuffer{}),
              0);
  });
}

TEST_F(TrpcClientStreamHandlerTest, SendMessageFailed) {
  RunAsFiber([&]() {
    MockSend(1);
    ASSERT_EQ(stream_handler_->SendMessage(stream_handler_->GetMutableStreamOptions()->context.context,
                                           NoncontiguousBuffer{}),
              TrpcRetCode::TRPC_STREAM_CLIENT_NETWORK_ERR);
  });
}

}  // namespace trpc::testing
