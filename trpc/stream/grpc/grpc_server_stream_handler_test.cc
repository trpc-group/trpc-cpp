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

#include "trpc/stream/grpc/grpc_server_stream_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/server_session.h"
#include "trpc/codec/grpc/http2/testing/mock_session.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/server/server_context.h"

namespace trpc::testing {

using namespace trpc::stream;

class GrpcServerStreamHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { trpc::codec::Init(); }

  static void TearDownTestSuite() { trpc::codec::Destroy(); }

  void SetUp() override {
    stream::StreamOptions stream_options_default;
    stream_options_default.send = [](IoMessage&& msg) { return 0; };
    stream_options_default.check_stream_rpc_function = [](const std::string& func_name) { return false; };
    default_stream_handler_ = MakeRefCounted<DefaultGrpcServerStreamHandler>(std::move(stream_options_default));

    stream::StreamOptions stream_options_fiber;
    stream_options_fiber.send = [](IoMessage&& msg) { return 0; };
    stream_options_fiber.check_stream_rpc_function = [](const std::string& func_name) { return false; };
    fiber_stream_handler_ = MakeRefCounted<FiberGrpcServerStreamHandler>(std::move(stream_options_fiber));

    http2_response_ = http2::CreateResponse();
    http2_response_->SetStatus(200);
    http2_response_->AddHeader("content-type", "application/json");
    http2_response_->AddHeader("x-user-defined", "Tom");
    http2_response_->AddHeader("x-user-defined", "Jerry");

    grpc_unary_response_ = std::make_shared<GrpcUnaryResponseProtocol>();
    grpc_unary_response_->SetHttp2Response(http2_response_);
  }

  void TearDown() override {}

  IoMessage CreateTransportMessage(const GrpcUnaryResponseProtocolPtr& rsp) {
    auto context = MakeRefCounted<ServerContext>();
    context->SetResponseMsg(rsp);
    IoMessage rsp_msg;
    rsp_msg.msg = context;
    return rsp_msg;
  }

 protected:
  RefPtr<stream::GrpcServerStreamHandler> default_stream_handler_{nullptr};
  RefPtr<stream::GrpcServerStreamHandler> fiber_stream_handler_{nullptr};
  GrpcUnaryResponseProtocolPtr grpc_unary_response_{nullptr};
  http2::ResponsePtr http2_response_{nullptr};
};

TEST_F(GrpcServerStreamHandlerTest, Init) { ASSERT_TRUE(default_stream_handler_->Init()); }

TEST_F(GrpcServerStreamHandlerTest, ParseMessage) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
  auto mock_session_ptr = mock_session.get();
  default_stream_handler_->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).Times(1);
  ASSERT_TRUE(default_stream_handler_->Init());

  EXPECT_CALL(*mock_session_ptr, SignalRead(::testing::_)).WillOnce([mock_session_ptr](NoncontiguousBuffer* in) {
    mock_session_ptr->OnRequest(http2::CreateRequest());
    return 0;
  });

  NoncontiguousBuffer in{};
  std::deque<std::any> out{};
  // Parses successfully.
  ASSERT_EQ(0, default_stream_handler_->ParseMessage(&in, &out));
  ASSERT_TRUE(!out.empty());
  // Failed to parse.
  EXPECT_CALL(*mock_session_ptr, SignalRead(::testing::_)).WillOnce(::testing::Return(-1));
  ASSERT_EQ(-1, default_stream_handler_->ParseMessage(&in, &out));
}

TEST_F(GrpcServerStreamHandlerTest, DefaultEncodeTransportMessageOk) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
  auto mock_session_ptr = mock_session.get();
  default_stream_handler_->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SubmitResponse(::testing::_)).WillOnce(::testing::Return(0));
  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));

  IoMessage response_msg = CreateTransportMessage(grpc_unary_response_);
  ASSERT_EQ(0, default_stream_handler_->EncodeTransportMessage(&response_msg));
}

TEST_F(GrpcServerStreamHandlerTest, DefaultEncodeTransportMessageFailedDueToBadAnyCastException) {
  IoMessage response_msg;
  ASSERT_EQ(-1, default_stream_handler_->EncodeTransportMessage(&response_msg));
}

TEST_F(GrpcServerStreamHandlerTest, DefaultEncodeTransportMessageFailedDueToSubmitResponseFailed) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
  auto mock_session_ptr = mock_session.get();
  default_stream_handler_->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SubmitResponse(::testing::_)).WillOnce(::testing::Return(-1));

  IoMessage response_msg = CreateTransportMessage(grpc_unary_response_);
  ASSERT_EQ(-1, default_stream_handler_->EncodeTransportMessage(&response_msg));
}

TEST_F(GrpcServerStreamHandlerTest, DefaultEncodeTransportMessageFailedDueToSignalWriteFailed) {
  auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
  auto mock_session_ptr = mock_session.get();
  default_stream_handler_->SetSession(std::move(mock_session));

  EXPECT_CALL(*mock_session_ptr, SubmitResponse(::testing::_)).WillOnce(::testing::Return(0));
  EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(-1));

  IoMessage response_msg = CreateTransportMessage(grpc_unary_response_);
  ASSERT_EQ(-1, default_stream_handler_->EncodeTransportMessage(&response_msg));
}

TEST_F(GrpcServerStreamHandlerTest, FiberEncodeTransportMessageOk) {
  RunAsFiber([&]() {
    auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
    auto mock_session_ptr = mock_session.get();
    fiber_stream_handler_->SetSession(std::move(mock_session));

    EXPECT_CALL(*mock_session_ptr, SubmitResponse(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));

    IoMessage response_msg = CreateTransportMessage(grpc_unary_response_);
    ASSERT_EQ(0, fiber_stream_handler_->EncodeTransportMessage(&response_msg));
  });
}

TEST_F(GrpcServerStreamHandlerTest, FiberEncodeTransportMessageFailedDueToBadAnyCastException) {
  RunAsFiber([&]() {
    IoMessage response_msg;
    ASSERT_EQ(-1, fiber_stream_handler_->EncodeTransportMessage(&response_msg));
  });
}

TEST_F(GrpcServerStreamHandlerTest, FiberEncodeTransportMessageFailedDueToSubmitResponseFailed) {
  RunAsFiber([&]() {
    auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
    auto mock_session_ptr = mock_session.get();
    fiber_stream_handler_->SetSession(std::move(mock_session));

    EXPECT_CALL(*mock_session_ptr, SubmitResponse(::testing::_)).WillOnce(::testing::Return(-1));

    IoMessage response_msg = CreateTransportMessage(grpc_unary_response_);
    ASSERT_EQ(-1, fiber_stream_handler_->EncodeTransportMessage(&response_msg));
  });
}

TEST_F(GrpcServerStreamHandlerTest, FiberEncodeTransportMessageFailedDueToSignalWriteFailed) {
  RunAsFiber([&]() {
    auto mock_session = std::make_unique<MockHttp2Session>(http2::SessionOptions());
    auto mock_session_ptr = mock_session.get();
    fiber_stream_handler_->SetSession(std::move(mock_session));

    EXPECT_CALL(*mock_session_ptr, SubmitResponse(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_ptr, SignalWrite(::testing::_)).WillOnce(::testing::Return(-1));

    IoMessage response_msg = CreateTransportMessage(grpc_unary_response_);
    ASSERT_EQ(-1, fiber_stream_handler_->EncodeTransportMessage(&response_msg));
  });
}

class FiberGrpcServerStreamHandlerTest : public ::testing::Test {
 public:
  using FiberGrpcServerStreamHandlerPtr = RefPtr<FiberGrpcServerStreamHandler>;
  FiberGrpcServerStreamHandlerPtr CreateFiberGrpcServerStreamHandler() {
    return MakeRefCounted<FiberGrpcServerStreamHandler>(StreamOptions());
  }

  static void SetUpTestSuite() { trpc::codec::Init(); }

  static void TearDownTestSuite() { trpc::codec::Destroy(); }

  void SetUp() override { stream_handler_ = CreateFiberGrpcServerStreamHandler(); }

  void TearDown() override {}

  void MockSend(int rc) {
    stream_handler_->GetMutableStreamOptions()->send = [rc](IoMessage&& msg) { return rc; };
  }

 protected:
  FiberGrpcServerStreamHandlerPtr stream_handler_;
};

TEST_F(FiberGrpcServerStreamHandlerTest, CreateStreamOk) {
  RunAsFiber([&]() {
    MockSend(0);

    stream::StreamOptions options;
    options.stream_id = 100;
    options.stream_handler = stream_handler_;
    options.context.context = MakeRefCounted<ServerContext>();
    options.server_mode = true;
    options.fiber_mode = true;
    auto stream = stream_handler_->CreateStream(std::move(options));
    EXPECT_TRUE(stream);

    // Terminate the coroutine by pushing RESET.
    stream_handler_->Stop();
    stream_handler_->Join();
  });
}

TEST_F(FiberGrpcServerStreamHandlerTest, SendMessageOk) {
  RunAsFiber([&]() {
    MockSend(0);
    ASSERT_EQ(stream_handler_->SendMessage(stream_handler_->GetMutableStreamOptions()->context.context,
                                           NoncontiguousBuffer{}),
              0);
  });
}

TEST_F(FiberGrpcServerStreamHandlerTest, SendMessageFailed) {
  RunAsFiber([&]() {
    MockSend(1);
    ASSERT_EQ(stream_handler_->SendMessage(stream_handler_->GetMutableStreamOptions()->context.context,
                                           NoncontiguousBuffer{}),
              GrpcStatus::kGrpcInternal);
  });
}

}  // namespace trpc::testing
