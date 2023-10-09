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

#include "trpc/stream/grpc/grpc_stream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/http2/testing/mock_session.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockGrpcStream : public GrpcStream {
 public:
  using MockState = GrpcStream::State;
  using MockEncodeMessageType = GrpcStream::EncodeMessageType;

  explicit MockGrpcStream(StreamOptions&& options, http2::Session* session, FiberMutex* mutex,
                          FiberConditionVariable* cv)
      : GrpcStream(std::move(options), session, mutex, cv) {}

  bool EncodeMessage(EncodeMessageType type, std::any&& msg, NoncontiguousBuffer* buffer) {
    return GrpcStream::EncodeMessage(type, std::move(msg), buffer);
  }

  State GetState() { return CommonStream::GetState(); }
};

using MockStreamPtr = RefPtr<MockGrpcStream>;
using MockState = MockGrpcStream::MockState;
using MockEncodeMessageType = MockGrpcStream::MockEncodeMessageType;

class GrpcStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StreamOptions options;
    options.stream_id = 1;
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;

    mock_session_ = CreateMockHttp2Session(http2::SessionOptions());

    mock_stream_ = MakeRefCounted<MockGrpcStream>(std::move(options), mock_session_.get(), &mutex_, &cv_);
  }

  void TearDown() override {}

 protected:
  MockStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
  MockHttp2SessionPtr mock_session_{nullptr};
  FiberMutex mutex_;
  FiberConditionVariable cv_;
};

TEST_F(GrpcStreamTest, GetErrorCodeTest) {
  RunAsFiber([&]() {
    ASSERT_EQ(mock_stream_->GetEncodeErrorCode(), GrpcStatus::kGrpcInternal);
    ASSERT_EQ(mock_stream_->GetDecodeErrorCode(), GrpcStatus::kGrpcInternal);
    ASSERT_EQ(mock_stream_->GetReadTimeoutErrorCode(), GrpcStatus::kGrpcDeadlineExceeded);
  });
}

TEST_F(GrpcStreamTest, EncodeHeaderOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockEncodeMessageType type = MockEncodeMessageType::kHeader;
    http2::HeaderPairs msg;
    NoncontiguousBuffer buffer;

    ASSERT_TRUE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));
  });
}

TEST_F(GrpcStreamTest, EncodeHeaderFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitReset(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockEncodeMessageType type = MockEncodeMessageType::kHeader;
    http2::HeaderPairs msg;
    NoncontiguousBuffer buffer;

    ASSERT_FALSE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), mock_stream_->GetEncodeErrorCode());
  });
}

TEST_F(GrpcStreamTest, EncodeDataOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitData_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockEncodeMessageType type = MockEncodeMessageType::kData;
    NoncontiguousBuffer msg;
    NoncontiguousBuffer buffer;

    ASSERT_TRUE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));
  });
}

TEST_F(GrpcStreamTest, EncodeDataFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitData_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitReset(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockEncodeMessageType type = MockEncodeMessageType::kData;
    NoncontiguousBuffer msg;
    NoncontiguousBuffer buffer;

    ASSERT_FALSE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), mock_stream_->GetEncodeErrorCode());
  });
}

TEST_F(GrpcStreamTest, EncodeTrailerOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitTrailer(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockEncodeMessageType type = MockEncodeMessageType::kTrailer;
    http2::TrailerPairs msg;
    NoncontiguousBuffer buffer;

    ASSERT_TRUE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));
  });
}

TEST_F(GrpcStreamTest, EncodeTrailerFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitTrailer(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitReset(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockEncodeMessageType type = MockEncodeMessageType::kTrailer;
    http2::TrailerPairs msg;
    NoncontiguousBuffer buffer;

    ASSERT_FALSE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), mock_stream_->GetEncodeErrorCode());
  });
}

TEST_F(GrpcStreamTest, EncodeResetOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitReset(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockEncodeMessageType type = MockEncodeMessageType::kReset;
    uint32_t msg = GrpcStatus::kGrpcInternal;
    NoncontiguousBuffer buffer;

    ASSERT_TRUE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));
  });
}

TEST_F(GrpcStreamTest, EncodeResetFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitReset(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).Times(0);

    MockEncodeMessageType type = MockEncodeMessageType::kReset;
    uint32_t msg = GrpcStatus::kGrpcInternal;
    NoncontiguousBuffer buffer;

    ASSERT_FALSE(mock_stream_->EncodeMessage(type, std::move(msg), &buffer));

    Status status = mock_stream_->GetStatus();
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), mock_stream_->GetEncodeErrorCode());
  });
}

}  // namespace trpc::testing
