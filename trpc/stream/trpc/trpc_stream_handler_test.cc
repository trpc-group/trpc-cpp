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

#include "trpc/stream/trpc/trpc_stream_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/trpc/testing/trpc_stream_testing.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockTrpcStream : public TrpcStream {
 public:
  explicit MockTrpcStream(StreamOptions&& options) : TrpcStream(std::move(options)) {}

  MOCK_METHOD1(PushRecvMessage, void(StreamRecvMessage&&));
};

class MockTrpcStreamHandler : public TrpcStreamHandler {
 public:
  explicit MockTrpcStreamHandler(StreamOptions&& options) : TrpcStreamHandler(std::move(options)) {}

  ~MockTrpcStreamHandler() override = default;

  RefPtr<MockTrpcStream> CreateStream(uint32_t id, bool fiber_mode) {
    StreamOptions opt;
    opt.stream_id = id;
    opt.fiber_mode = fiber_mode;

    auto stream = MakeRefCounted<MockTrpcStream>(std::move(opt));
    return CriticalSection<RefPtr<MockTrpcStream>>([&]() {
      streams_.emplace(id, stream);
      return stream;
    });
  }

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override { return nullptr; }
  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override { return 0; }
};

using MockTrpcStreamHandlerPtr = RefPtr<MockTrpcStreamHandler>;

class TrpcStreamHandlerTest : public ::testing::Test {
 public:
  using TrpcStreamHandlerPtr = std::shared_ptr<TrpcStreamHandler>;

  void SetUp() override {
    StreamOptions options;
    stream_handler_ = MakeRefCounted<MockTrpcStreamHandler>(std::move(options));
  }

  void TearDown() override {}

 protected:
  MockTrpcStreamHandlerPtr stream_handler_{nullptr};
};

namespace {
// TRPC_STREAM_FRAME_INIT = 0x01
// TRPC_STREAM_FRAME_DATA = 0x02

const int kTrpcStreamFrameInit = 0x01;
const int kTrpcStreamFrameData = 0x02;
}  // namespace

TEST_F(TrpcStreamHandlerTest, TestInit) { ASSERT_TRUE(stream_handler_->Init()); }

TEST_F(TrpcStreamHandlerTest, RemoveStreamTest) {
  RunAsFiber([&]() {
    stream_handler_->CreateStream(100, true);
    ASSERT_FALSE(stream_handler_->IsNewStream(100, kTrpcStreamFrameInit));
    stream_handler_->RemoveStream(100);
    ASSERT_TRUE(stream_handler_->IsNewStream(100, kTrpcStreamFrameInit));
  });
}

TEST_F(TrpcStreamHandlerTest, IsNewStreamTest) {
  RunAsFiber([&]() {
    ASSERT_FALSE(stream_handler_->IsNewStream(100, kTrpcStreamFrameData));
    ASSERT_TRUE(stream_handler_->IsNewStream(100, kTrpcStreamFrameInit));

    stream_handler_->CreateStream(100, true);
    ASSERT_FALSE(stream_handler_->IsNewStream(100, kTrpcStreamFrameInit));
  });
}

TEST_F(TrpcStreamHandlerTest, PushMessage) {
  RunAsFiber([&]() {
    auto stream = stream_handler_->CreateStream(100, true);
    EXPECT_CALL(*stream, PushRecvMessage(::testing::_)).Times(1);

    ProtocolMessageMetadata meta1;
    meta1.stream_id = 1;  // nonexist stream
    stream_handler_->PushMessage(std::any{}, std::move(meta1));

    ProtocolMessageMetadata meta2;
    meta2.stream_id = 100;
    stream_handler_->PushMessage(std::any{}, std::move(meta2));
  });
}

TEST_F(TrpcStreamHandlerTest, PushMessageOnThread) {
  auto stream = stream_handler_->CreateStream(100, false);
  EXPECT_CALL(*stream, PushRecvMessage(::testing::_)).Times(1);

  ProtocolMessageMetadata meta1;
  meta1.stream_id = 1;  // nonexist stream
  stream_handler_->PushMessage(std::any{}, std::move(meta1));

  ProtocolMessageMetadata meta2;
  meta2.stream_id = 100;
  stream_handler_->PushMessage(std::any{}, std::move(meta2));
}

}  // namespace trpc::testing
