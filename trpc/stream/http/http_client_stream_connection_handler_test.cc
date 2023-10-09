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

#include "trpc/stream/http/http_client_stream_connection_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/client_codec.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/testing/client_codec_testing.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/http/http_client_stream_handler.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/util/http/stream/http_client_stream_response.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class FiberHttpClientStreamConnectionHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    conn_ = std::make_unique<MockTcpConnection>();

    trans_info_ = std::make_unique<TransInfo>();
    trans_info_->protocol = "MockClientCodec";
    trans_info_->checker_function = [](const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) { return 0; };

    conn_handler_ = std::make_unique<FiberHttpClientStreamConnectionHandler>(conn_.get(), trans_info_.get());
    conn_handler_->SetMsgHandleFunc([](const ConnectionPtr&, std::deque<std::any>&) { return true; });

    codec_ = std::make_shared<MockClientCodec>();
    stream_handler_ = MakeRefCounted<HttpClientStreamHandler>(StreamOptions{});

    ClientCodecFactory::GetInstance()->Register(codec_);
    ClientStreamHandlerFactory::GetInstance()->Register(
        "MockClientCodec", [&](StreamOptions&& options) -> StreamHandlerPtr { return stream_handler_; });
  }

  void TearDown() override {
    ClientCodecFactory::GetInstance()->Clear();
    ClientStreamHandlerFactory::GetInstance()->Clear();
  }

 protected:
  std::unique_ptr<FiberHttpClientStreamConnectionHandler> conn_handler_{nullptr};
  std::unique_ptr<MockTcpConnection> conn_{nullptr};
  std::unique_ptr<TransInfo> trans_info_{nullptr};
  std::shared_ptr<MockClientCodec> codec_{nullptr};
  HttpClientStreamHandlerPtr stream_handler_{nullptr};
};

TEST_F(FiberHttpClientStreamConnectionHandlerTest, GetOrCreateStreamHandler) {
  RunAsFiber([&]() {
    StreamHandlerPtr stream_handler = conn_handler_->GetOrCreateStreamHandler();

    StreamHandlerPtr check_stream_handler = conn_handler_->GetOrCreateStreamHandler();

    ASSERT_EQ(stream_handler.get(), check_stream_handler.get());
  });
}

TEST_F(FiberHttpClientStreamConnectionHandlerTest, CheckMessage) {
  RunAsFiber([&]() {
    ConnectionPtr conn = MakeRefCounted<Connection>();
    NoncontiguousBuffer check_in;
    std::deque<std::any> check_out;
    // If there is no stream_handler on the connection, the expectation is that the check process will not set the
    // HttpClientStreamResponse variable.
    ASSERT_EQ(0, conn_handler_->CheckMessage(conn, check_in, check_out));
    auto p = std::any_cast<HttpClientStreamResponsePtr>(&(conn->GetUserAny()));
    ASSERT_FALSE(p);

    conn_handler_->GetOrCreateStreamHandler();
    // If the `stream_handler` exists on the connection, it is expected that the check process will set the
    // `HttpClientStreamResponse` variable and that the `Stream` object will be consistent with the `stream_handler`.
    ASSERT_EQ(0, conn_handler_->CheckMessage(conn, check_in, check_out));
    p = std::any_cast<HttpClientStreamResponsePtr>(&(conn->GetUserAny()));
    ASSERT_TRUE(p);
    ASSERT_EQ((*p)->GetStream().get(), stream_handler_->GetHttpStream().get());
    ASSERT_FALSE(stream_handler_->GetHttpStream().get());

    stream_handler_->CreateStream(StreamOptions{});
    ASSERT_EQ(0, conn_handler_->CheckMessage(conn, check_in, check_out));
    p = std::any_cast<HttpClientStreamResponsePtr>(&(conn->GetUserAny()));
    ASSERT_TRUE(p);
    ASSERT_EQ((*p)->GetStream().get(), stream_handler_->GetHttpStream().get());
    ASSERT_TRUE(stream_handler_->GetHttpStream().get());

    stream_handler_->RemoveStream(0);
    ASSERT_FALSE(stream_handler_->GetHttpStream().get());
  });
}

TEST_F(FiberHttpClientStreamConnectionHandlerTest, HandleMessage) {
  RunAsFiber([&]() {
    ConnectionPtr conn = MakeRefCounted<Connection>();
    std::deque<std::any> rsp_list;
    rsp_list.push_back("stream");
    ASSERT_TRUE(conn_handler_->HandleMessage(conn, rsp_list));

    stream_handler_->CreateStream(StreamOptions{});
    rsp_list.push_back("stream");
    ASSERT_TRUE(conn_handler_->HandleMessage(conn, rsp_list));
    stream_handler_->RemoveStream(0);
    ASSERT_FALSE(stream_handler_->GetHttpStream().get());
  });
}

}  // namespace trpc::testing
