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

#include "trpc/stream/fiber_stream_job_scheduler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/stream_provider.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockStreamHandler : public StreamHandler {
 public:
  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override { return nullptr; }

  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override { return false; }

  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override {}

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override { return -1; }

  MOCK_METHOD(int, RemoveStream, (uint32_t stream_id));
};

class FakeStream : public StreamReaderWriterProvider {
 public:
  explicit FakeStream(StreamOptions&& options) : options_(std::move(options)) {}

  Status Read(NoncontiguousBuffer* msg, int timeout = -1) override { return GetStatus(); }

  Status Write(NoncontiguousBuffer&& msg) override { return GetStatus(); }

  Status WriteDone() override { return GetStatus(); }

  Status Start() override { return GetStatus(); }

  Status Finish() override { return GetStatus(); }

  void Close(Status status) override {}

  void Reset(Status status) override {}

  Status GetStatus() const override { return kSuccStatus; }

  StreamOptions* GetMutableStreamOptions() override { return &options_; }

 private:
  StreamOptions options_;
};

class FiberStreamJobSchedulerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_stream_handler_ = MakeRefCounted<MockStreamHandler>();
    StreamOptions options;
    options.stream_handler = mock_stream_handler_;
    stream_ = MakeRefCounted<FakeStream>(std::move(options));
    job_scheduler_ = MakeRefCounted<FiberStreamJobScheduler>(
        1, stream_.Get(),
        [this](StreamRecvMessage&&) {
          if (recv_error_mode_) return RetCode::kError;
          ++recv_;
          return RetCode::kSuccess;
        },
        [this](StreamSendMessage&&) {
          if (send_error_mode_) return RetCode::kError;
          ++send_;
          return RetCode::kSuccess;
        });
  }

  void TearDown() override {
    recv_ = 0;
    send_ = 0;
  }

 protected:
  RefPtr<FiberStreamJobScheduler> job_scheduler_;
  RefPtr<FakeStream> stream_;
  RefPtr<MockStreamHandler> mock_stream_handler_;

  std::atomic_bool recv_error_mode_{false};
  std::atomic_bool send_error_mode_{false};

  std::atomic_int recv_{0};
  std::atomic_int send_{0};
};

TEST_F(FiberStreamJobSchedulerTest, HandleSendRecvMessageSuccess) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    ASSERT_FALSE(job_scheduler_->IsTerminate());
    ASSERT_EQ(recv_, 0);
    ASSERT_EQ(send_, 0);

    StreamRecvMessage recv_msg;
    StreamSendMessage send_msg;

    job_scheduler_->PushRecvMessage(std::move(recv_msg));
    job_scheduler_->PushSendMessage(std::move(send_msg));

    job_scheduler_->Stop();
    job_scheduler_->Join();

    ASSERT_TRUE(job_scheduler_->IsTerminate());
    ASSERT_EQ(recv_, 1);
    ASSERT_EQ(send_, 1);
    ASSERT_EQ(job_scheduler_->GetRecvQueueSize(), 0);
    ASSERT_EQ(job_scheduler_->GetSendQueueSize(), 0);
  });
}

TEST_F(FiberStreamJobSchedulerTest, HandleSendErrorTest) {
  send_error_mode_ = true;

  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    StreamSendMessage send_msg;

    job_scheduler_->PushRecvMessage(std::move(recv_msg));
    job_scheduler_->PushSendMessage(std::move(send_msg));

    // automatically stopped
    job_scheduler_->Join();

    ASSERT_TRUE(job_scheduler_->IsTerminate());
    ASSERT_EQ(recv_, 1);
    ASSERT_EQ(send_, 0);
    ASSERT_EQ(job_scheduler_->GetRecvQueueSize(), 0);
    ASSERT_EQ(job_scheduler_->GetSendQueueSize(), 0);
  });
}

TEST_F(FiberStreamJobSchedulerTest, HandleRecvErrorTest) {
  recv_error_mode_ = true;

  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    StreamSendMessage send_msg;

    job_scheduler_->PushRecvMessage(std::move(recv_msg));
    job_scheduler_->PushSendMessage(std::move(send_msg));

    // automatically stopped
    job_scheduler_->Join();

    ASSERT_TRUE(job_scheduler_->IsTerminate());
    ASSERT_EQ(send_, 0);
    ASSERT_EQ(recv_, 0);
    ASSERT_EQ(job_scheduler_->GetRecvQueueSize(), 0);
    ASSERT_EQ(job_scheduler_->GetSendQueueSize(), 0);
  });
}

TEST_F(FiberStreamJobSchedulerTest, StreamStopPushMessageFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    job_scheduler_->Stop();

    StreamRecvMessage recv_msg;
    StreamSendMessage send_msg;

    job_scheduler_->PushRecvMessage(std::move(recv_msg));
    job_scheduler_->PushSendMessage(std::move(send_msg));

    job_scheduler_->Join();

    ASSERT_TRUE(job_scheduler_->IsTerminate());
    ASSERT_EQ(send_, 0);
    ASSERT_EQ(recv_, 0);
    ASSERT_EQ(job_scheduler_->GetRecvQueueSize(), 0);
    ASSERT_EQ(job_scheduler_->GetSendQueueSize(), 0);
  });
}

}  // namespace trpc::testing
