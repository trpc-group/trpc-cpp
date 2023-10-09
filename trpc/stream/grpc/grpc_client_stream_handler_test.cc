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

#include "trpc/stream/grpc/grpc_client_stream_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/testing/mock_session.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc::testing {

using namespace trpc::stream;

class GrpcClientStreamHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { trpc::codec::Init(); }

  static void TearDownTestSuite() { trpc::codec::Destroy(); }

  void SetUp() override {
    stream::StreamOptions stream_options_default;
    stream_options_default.send = [](IoMessage&& msg) { return 0; };
    default_stream_handler_ = MakeRefCounted<GrpcDefaultClientStreamHandler>(std::move(stream_options_default));

    stream::StreamOptions stream_options_fiber;
    stream_options_fiber.send = [](IoMessage&& msg) { return 0; };
    fiber_stream_handler_ = MakeRefCounted<GrpcFiberClientStreamHandler>(std::move(stream_options_fiber));

    http2_request_ = http2::CreateRequest();
    http2_request_->SetMethod("GET");
    http2_request_->SetPath("/xx.yy.zz/SayHello");
    http2_request_->SetScheme("http");
    http2_request_->SetAuthority("127.0.0.1");

    grpc_unary_request_ = std::make_shared<GrpcUnaryRequestProtocol>();
    grpc_unary_request_->SetHttp2Request(http2_request_);
  }

  void TearDown() override {}

  IoMessage CreateTransportMessage(const GrpcUnaryRequestProtocolPtr& req) {
    auto context = MakeRefCounted<ClientContext>();
    context->SetRequest(req);
    IoMessage rsp_msg;
    rsp_msg.msg = context;
    return rsp_msg;
  }

 protected:
  RefPtr<stream::GrpcDefaultClientStreamHandler> default_stream_handler_{nullptr};
  RefPtr<stream::GrpcFiberClientStreamHandler> fiber_stream_handler_{nullptr};
  GrpcUnaryRequestProtocolPtr grpc_unary_request_{nullptr};
  http2::RequestPtr http2_request_{nullptr};
};

TEST_F(GrpcClientStreamHandlerTest, Init) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
  auto mock_session_ptr = mock_session.get();
  int send_ok{0};
  stream::StreamOptions stream_options{};
  stream_options.send = [&send_ok](IoMessage&& msg) { return send_ok; };

  auto stream_handler = MakeRefCounted<GrpcDefaultClientStreamHandler>(std::move(stream_options));
  stream_handler->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
  ASSERT_TRUE(stream_handler->Init());

  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_FALSE(stream_handler->Init());

  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
  send_ok = -1;
  ASSERT_FALSE(stream_handler->Init());
}

TEST_F(GrpcClientStreamHandlerTest, ParseMessage) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions{});
  auto mock_session_ptr = mock_session.get();
  default_stream_handler_->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
  ASSERT_TRUE(default_stream_handler_->Init());

  EXPECT_CALL(*mock_session_ptr, SignalRead(::testing::_)).WillOnce([mock_session_ptr](NoncontiguousBuffer* in) {
    mock_session_ptr->OnResponse(http2::CreateResponse());
    return 0;
  });

  NoncontiguousBuffer in{};
  std::deque<std::any> out{};

  ASSERT_EQ(0, default_stream_handler_->ParseMessage(&in, &out));
  ASSERT_FALSE(out.empty());

  EXPECT_CALL(*mock_session_ptr, SignalRead(::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_EQ(-1, default_stream_handler_->ParseMessage(&in, &out));
}

TEST_F(GrpcClientStreamHandlerTest, DefaultEncodeTransportMessageOk) {
  IoMessage request_msg = CreateTransportMessage(grpc_unary_request_);
  ASSERT_EQ(0, default_stream_handler_->EncodeTransportMessage(&request_msg));
}

TEST_F(GrpcClientStreamHandlerTest, DefaultEncodeTransportMessageFailedDueToBadAnyCastException) {
  IoMessage request_msg;
  ASSERT_EQ(-1, default_stream_handler_->EncodeTransportMessage(&request_msg));
}

TEST_F(GrpcClientStreamHandlerTest, DefaultEncodeTransportMessageFailedDueToSubmitRequestFailed) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
  auto mock_session_ptr = mock_session.get();
  default_stream_handler_->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SubmitRequest(::testing::_)).WillOnce(::testing::Return(-1));

  IoMessage request_msg = CreateTransportMessage(grpc_unary_request_);
  ASSERT_EQ(-1, default_stream_handler_->EncodeTransportMessage(&request_msg));
}

TEST_F(GrpcClientStreamHandlerTest, DefaultEncodeTransportMessageFailedDueToSignalWriteFailed) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
  auto mock_session_ptr = mock_session.get();
  default_stream_handler_->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SubmitRequest(::testing::_)).WillOnce(::testing::Return(0));
  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(-1));

  IoMessage request_msg = CreateTransportMessage(grpc_unary_request_);
  ASSERT_EQ(-1, default_stream_handler_->EncodeTransportMessage(&request_msg));
}

TEST_F(GrpcClientStreamHandlerTest, FiberEncodeTransportMessageOk) {
  RunAsFiber([&]() {
    IoMessage request_msg = CreateTransportMessage(grpc_unary_request_);
    ASSERT_EQ(0, fiber_stream_handler_->EncodeTransportMessage(&request_msg));
  });
}

TEST_F(GrpcClientStreamHandlerTest, FiberEncodeTransportMessageFailedDueToBadAnyCastException) {
  RunAsFiber([&]() {
    IoMessage request_msg;
    ASSERT_EQ(-1, fiber_stream_handler_->EncodeTransportMessage(&request_msg));
  });
}

TEST_F(GrpcClientStreamHandlerTest, FiberEncodeTransportMessageFailedDueToSubmitRequestFailed) {
  RunAsFiber([&]() {
    auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
    auto mock_session_ptr = mock_session.get();
    fiber_stream_handler_->SetSession(std::move(mock_session));

    EXPECT_CALL(*mock_session_ptr, SubmitRequest(::testing::_)).WillOnce(::testing::Return(-1));

    IoMessage request_msg = CreateTransportMessage(grpc_unary_request_);
    ASSERT_EQ(-1, fiber_stream_handler_->EncodeTransportMessage(&request_msg));
  });
}

TEST_F(GrpcClientStreamHandlerTest, FiberEncodeTransportMessageFailedDueToSignalWriteFailed) {
  RunAsFiber([&]() {
    auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
    auto mock_session_ptr = mock_session.get();
    fiber_stream_handler_->SetSession(std::move(mock_session));

    EXPECT_CALL(*mock_session_ptr, SubmitRequest(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(-1));

    IoMessage request_msg = CreateTransportMessage(grpc_unary_request_);
    ASSERT_EQ(-1, fiber_stream_handler_->EncodeTransportMessage(&request_msg));
  });
}

}  // namespace trpc::testing
