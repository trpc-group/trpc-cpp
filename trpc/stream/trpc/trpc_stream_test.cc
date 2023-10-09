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

#include "trpc/stream/trpc/trpc_stream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockTrpcStream : public TrpcStream {
 public:
  using MockState = TrpcStream::State;

  explicit MockTrpcStream(StreamOptions&& options) : TrpcStream(std::move(options)) {}

  bool EncodeMessage(Protocol* protocol, NoncontiguousBuffer* buffer) {
    return TrpcStream::EncodeMessage(protocol, buffer);
  }

  bool DecodeMessage(NoncontiguousBuffer* buffer, Protocol* protocol) {
    return TrpcStream::DecodeMessage(buffer, protocol);
  }

  RetCode PushSendMessage(StreamSendMessage&& msg, bool push_front = false) override {
    if (push_front) {
      is_push_front = true;
    }
    if (msg.category == StreamMessageCategory::kStreamFeedback) {
      auto feedback_meta = std::any_cast<TrpcStreamFeedBackMeta>(msg.metadata);
      feedback_increment = feedback_meta.window_size_increment();
    }
    return RetCode::kSuccess;
  }

  State GetState() { return CommonStream::GetState(); }

  void SetRecvFlowController(RefPtr<TrpcStreamRecvController> recv_controller) {
    recv_flow_controller_ = recv_controller;
  }
  void SetSendFlowController(RefPtr<TrpcStreamSendController> send_controller) {
    send_flow_controller_ = send_controller;
  }

  void TriggerOnData(NoncontiguousBuffer&& msg) { CommonStream::OnData(std::move(msg)); }

  void NotifyFlowControlCond() { flow_control_cond_.notify_all(); }

 public:
  uint32_t feedback_increment = 0;
  bool is_push_front = false;
};

using MockStreamPtr = RefPtr<MockTrpcStream>;
using MockState = MockTrpcStream::MockState;

class TrpcStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StreamOptions options;
    options.stream_id = 1;
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;
    options.fiber_mode = true;
    mock_stream_ = MakeRefCounted<MockTrpcStream>(std::move(options));
  }

  void TearDown() override {}

 protected:
  MockStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
};

TEST_F(TrpcStreamTest, EncodeOkTest) {
  RunAsFiber([&]() {
    MockProtocol protocol;
    NoncontiguousBuffer buffer;
    EXPECT_CALL(protocol, ZeroCopyEncode(::testing::_)).WillOnce(::testing::Return(true));

    ASSERT_TRUE(mock_stream_->EncodeMessage(&protocol, &buffer));
  });
}

TEST_F(TrpcStreamTest, EncodeFailedTest) {
  RunAsFiber([&]() {
    MockProtocol protocol;
    NoncontiguousBuffer buffer;

    EXPECT_CALL(protocol, ZeroCopyEncode(::testing::_)).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    ASSERT_FALSE(mock_stream_->EncodeMessage(&protocol, &buffer));
    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), mock_stream_->GetDecodeErrorCode());
  });
}

TEST_F(TrpcStreamTest, DecodeOkTest) {
  RunAsFiber([&]() {
    MockProtocol protocol;
    NoncontiguousBuffer buffer;
    EXPECT_CALL(protocol, ZeroCopyDecode(::testing::_)).WillOnce(::testing::Return(true));

    ASSERT_TRUE(mock_stream_->DecodeMessage(&buffer, &protocol));
  });
}

TEST_F(TrpcStreamTest, DecodeFailedTest) {
  RunAsFiber([&]() {
    MockProtocol protocol;
    NoncontiguousBuffer buffer;

    EXPECT_CALL(protocol, ZeroCopyDecode(::testing::_)).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    ASSERT_FALSE(mock_stream_->DecodeMessage(&buffer, &protocol));
    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), mock_stream_->GetEncodeErrorCode());
  });
}

NoncontiguousBuffer BuildBuffer(int len) {
  NoncontiguousBufferBuilder builder;
  builder.Append(std::string(len, 's'));
  return builder.DestructiveGet();
}

TEST_F(TrpcStreamTest, TestReadWithFlowControl) {
  RunAsFiber([&]() {
    auto recv_controller = MakeRefCounted<TrpcStreamRecvController>(100);
    mock_stream_->SetRecvFlowController(recv_controller);
    // If less than 1/4 is read, no feedback will be sent.
    mock_stream_->TriggerOnData(BuildBuffer(10));
    NoncontiguousBuffer msg;
    Status status = mock_stream_->Read(&msg, -1);
    ASSERT_TRUE(status.OK());
    ASSERT_EQ(mock_stream_->feedback_increment, 0);
    ASSERT_FALSE(mock_stream_->is_push_front);
    // If greater than 1/4 is read, a feedback will be sent.
    mock_stream_->TriggerOnData(BuildBuffer(20));
    status = mock_stream_->Read(&msg, -1);
    ASSERT_TRUE(status.OK());
    ASSERT_EQ(mock_stream_->feedback_increment, 30);
    ASSERT_TRUE(mock_stream_->is_push_front);
  });
}

TEST_F(TrpcStreamTest, TestWriteWithFlowControl) {
  RunAsFiber([&]() {
    auto write_controller = MakeRefCounted<TrpcStreamSendController>(100);
    mock_stream_->SetSendFlowController(write_controller);
    ::trpc::FiberLatch l(1);
    int block_interval = 0;
    ::trpc::StartFiberDetached([&]() {
      // The first send will be ok.
      mock_stream_->Write(BuildBuffer(110));
      // The second send will be blocking: wait-interval.
      auto t_start = std::chrono::high_resolution_clock::now();
      mock_stream_->Write(BuildBuffer(30));
      auto t_end = std::chrono::high_resolution_clock::now();
      block_interval = std::chrono::duration<double, std::milli>(t_end - t_start).count();
      l.CountDown();
    });
    ::trpc::FiberSleepFor(std::chrono::milliseconds(1000));
    mock_stream_->NotifyFlowControlCond();
    l.Wait();
    ASSERT_GE(block_interval, 900);
  });
}

}  // namespace trpc::testing
