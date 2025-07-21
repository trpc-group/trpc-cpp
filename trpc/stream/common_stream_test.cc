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

#include "trpc/stream/common_stream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_worker_thread.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/util/thread/latch.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockCommonStream : public CommonStream {
 public:
  using MockRetCode = FiberStreamJobScheduler::RetCode;
  using MockAction = CommonStream::Action;
  using MockState = CommonStream::State;

  explicit MockCommonStream(StreamOptions&& options) : CommonStream(std::move(options)) {}

  MOCK_METHOD(RetCode, HandleInit_rvr, (StreamRecvMessage &&));
  MOCK_METHOD(RetCode, HandleData_rvr, (StreamRecvMessage &&));
  MOCK_METHOD(RetCode, HandleFeedback_rvr, (StreamRecvMessage &&));
  MOCK_METHOD(RetCode, HandleClose_rvr, (StreamRecvMessage &&));
  MOCK_METHOD(RetCode, HandleReset_rvr, (StreamRecvMessage &&));

  MOCK_METHOD(RetCode, SendInit_rvr, (StreamSendMessage &&));
  MOCK_METHOD(RetCode, SendData_rvr, (StreamSendMessage &&));
  MOCK_METHOD(RetCode, SendFeedback_rvr, (StreamSendMessage &&));
  MOCK_METHOD(RetCode, SendClose_rvr, (StreamSendMessage &&));
  MOCK_METHOD(RetCode, SendReset_rvr, (StreamSendMessage &&));

  // Add a Reset message for unit testing to the queue. In actual implementation, the corresponding Reset message
  // will be passed in based on the protocol.
  void Reset(Status status) override {
    StreamSendMessage send_msg;
    send_msg.category = StreamMessageCategory::kStreamReset;
    PushSendMessage(std::move(send_msg));
    TRPC_LOG_INFO("xxxxx");
  }

  void OnReady() { CommonStream::OnReady(); }
  void OnData(NoncontiguousBuffer&& msg) { CommonStream::OnData(std::move(msg)); }
  void OnFinish(Status status) { CommonStream::OnFinish(status); }
  void OnError(Status status) { CommonStream::OnError(status); }

  static std::string_view StreamStateToString(State state) { return CommonStream::StreamStateToString(state); }
  static std::string_view StreamActionToString(Action action) { return CommonStream::StreamActionToString(action); }
  static std::string_view StreamMessageCategoryToString(StreamMessageCategory c) {
    return CommonStream::StreamMessageCategoryToString(c);
  }

  State GetState() { return CommonStream::GetState(); }
  void SetState(State state) { CommonStream::SetState(state); }
  const StreamVarPtr& GetStreamVar() const { return CommonStream::GetStreamVar(); }
  void SetStreamVar(StreamVarPtr&& stream_var) { CommonStream::SetStreamVar(std::move(stream_var)); }

  bool Send(NoncontiguousBuffer&& buffer) { return CommonStream::Send(std::move(buffer)); }

 protected:
  RetCode HandleInit(StreamRecvMessage&& msg) override { return HandleInit_rvr(std::move(msg)); }
  RetCode HandleData(StreamRecvMessage&& msg) override { return HandleData_rvr(std::move(msg)); }
  RetCode HandleFeedback(StreamRecvMessage&& msg) override { return HandleFeedback_rvr(std::move(msg)); }
  RetCode HandleClose(StreamRecvMessage&& msg) override { return HandleClose_rvr(std::move(msg)); }
  RetCode HandleReset(StreamRecvMessage&& msg) override { return HandleReset_rvr(std::move(msg)); }

  RetCode SendInit(StreamSendMessage&& msg) override { return SendInit_rvr(std::move(msg)); }
  RetCode SendData(StreamSendMessage&& msg) override { return SendData_rvr(std::move(msg)); }
  RetCode SendFeedback(StreamSendMessage&& msg) override { return SendFeedback_rvr(std::move(msg)); }
  RetCode SendClose(StreamSendMessage&& msg) override { return SendClose_rvr(std::move(msg)); }
  RetCode SendReset(StreamSendMessage&& msg) override { return SendReset_rvr(std::move(msg)); }
};

using MockRetCode = MockCommonStream::MockRetCode;
using MockAction = MockCommonStream::MockAction;
using MockState = MockCommonStream::MockState;
using MockStreamPtr = RefPtr<MockCommonStream>;

class CommonStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StreamOptions options;
    options.stream_id = 1;
    options.fiber_mode = true;
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;
    mock_stream_ = MakeRefCounted<MockCommonStream>(std::move(options));
  }

  void TearDown() override {}

  void SetStreamHandlerMockExpect() { EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1); }

 protected:
  MockStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
};

TEST_F(CommonStreamTest, StreamMessageCategoryToStringTest) {
  ASSERT_EQ(mock_stream_->StreamMessageCategoryToString(StreamMessageCategory::kStreamInit), "stream_init");
  ASSERT_EQ(mock_stream_->StreamMessageCategoryToString(StreamMessageCategory::kStreamData), "stream_data");
  ASSERT_EQ(mock_stream_->StreamMessageCategoryToString(StreamMessageCategory::kStreamFeedback), "stream_feedback");
  ASSERT_EQ(mock_stream_->StreamMessageCategoryToString(StreamMessageCategory::kStreamClose), "stream_close");
  ASSERT_EQ(mock_stream_->StreamMessageCategoryToString(StreamMessageCategory::kStreamUnknown), "stream_unknown");
}

TEST_F(CommonStreamTest, StreamStateToStringTest) {
  ASSERT_EQ(mock_stream_->StreamStateToString(MockState::kClosed), "closed");
  ASSERT_EQ(mock_stream_->StreamStateToString(MockState::kIdle), "idle");
  ASSERT_EQ(mock_stream_->StreamStateToString(MockState::kInit), "init");
  ASSERT_EQ(mock_stream_->StreamStateToString(MockState::kOpen), "open");
  ASSERT_EQ(mock_stream_->StreamStateToString(MockState::kLocalClosed), "local_closed");
  ASSERT_EQ(mock_stream_->StreamStateToString(MockState::kRemoteClosed), "remote_closed");
}

TEST_F(CommonStreamTest, StreamActionToStringTest) {
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kSendInit), "send init");
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kSendData), "send data");
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kSendFeedback), "send feedback");
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kSendClose), "send close");
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kHandleInit), "handle init");
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kHandleData), "handle data");
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kHandleFeedback), "handle feedback");
  ASSERT_EQ(mock_stream_->StreamActionToString(MockAction::kHandleClose), "handle close");
}

TEST_F(CommonStreamTest, SetAndGetStateTest) {
  mock_stream_->SetState(MockState::kInit);
  ASSERT_EQ(mock_stream_->GetState(), MockState::kInit);
}

TEST_F(CommonStreamTest, SetAndGetStreamVarTest) {
  StreamVarPtr stream_var = StreamVarHelper::GetInstance()->GetOrCreateStreamVar("xxxx");
  StreamVar* check_stream_var = stream_var.Get();
  mock_stream_->SetStreamVar(std::move(stream_var));
  ASSERT_EQ(mock_stream_->GetStreamVar().Get(), check_stream_var);
}

TEST_F(CommonStreamTest, HandleNormalSendRecvMessageTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);
    SetStreamHandlerMockExpect();
    StartFiberDetached([&]() {
      EXPECT_CALL(*mock_stream_, HandleInit_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, HandleData_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, HandleFeedback_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, HandleClose_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, HandleReset_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));

      EXPECT_CALL(*mock_stream_, SendInit_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, SendData_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, SendFeedback_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, SendClose_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));
      EXPECT_CALL(*mock_stream_, SendReset_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kSuccess));

      StreamRecvMessage recv_msg_init;
      recv_msg_init.metadata.stream_frame_type = static_cast<uint8_t>(StreamMessageCategory::kStreamInit);
      StreamRecvMessage recv_msg_data;
      recv_msg_data.metadata.stream_frame_type = static_cast<uint8_t>(StreamMessageCategory::kStreamData);
      StreamRecvMessage recv_msg_feedback;
      recv_msg_feedback.metadata.stream_frame_type = static_cast<uint8_t>(StreamMessageCategory::kStreamFeedback);
      StreamRecvMessage recv_msg_close;
      recv_msg_close.metadata.stream_frame_type = static_cast<uint8_t>(StreamMessageCategory::kStreamClose);
      StreamRecvMessage recv_msg_reset;
      recv_msg_reset.metadata.stream_frame_type = static_cast<uint8_t>(StreamMessageCategory::kStreamReset);

      StreamSendMessage send_msg_init;
      send_msg_init.category = StreamMessageCategory::kStreamInit;
      StreamSendMessage send_msg_data;
      send_msg_data.category = StreamMessageCategory::kStreamData;
      StreamSendMessage send_msg_feedback;
      send_msg_feedback.category = StreamMessageCategory::kStreamFeedback;
      StreamSendMessage send_msg_close;
      send_msg_close.category = StreamMessageCategory::kStreamClose;
      StreamSendMessage send_msg_reset;
      send_msg_reset.category = StreamMessageCategory::kStreamReset;

      mock_stream_->PushRecvMessage(std::move(recv_msg_init));
      mock_stream_->PushRecvMessage(std::move(recv_msg_data));
      mock_stream_->PushRecvMessage(std::move(recv_msg_feedback));
      mock_stream_->PushRecvMessage(std::move(recv_msg_close));
      mock_stream_->PushRecvMessage(std::move(recv_msg_reset));

      mock_stream_->PushSendMessage(std::move(send_msg_init));
      mock_stream_->PushSendMessage(std::move(send_msg_data));
      mock_stream_->PushSendMessage(std::move(send_msg_feedback));
      mock_stream_->PushSendMessage(std::move(send_msg_close));
      mock_stream_->PushSendMessage(std::move(send_msg_reset));

      mock_stream_->Stop();
      mock_stream_->Join();

      fiber_latch.CountDown();
    });

    fiber_latch.Wait();

    ASSERT_TRUE(mock_stream_->IsTerminate());
  });
}

TEST_F(CommonStreamTest, HandleUnknownRecvMessageTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);
    SetStreamHandlerMockExpect();
    StartFiberDetached([&]() {
      EXPECT_CALL(*mock_stream_, SendReset_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kError));

      StreamRecvMessage recv_msg_unknown;
      recv_msg_unknown.metadata.stream_frame_type = static_cast<uint8_t>(StreamMessageCategory::kStreamUnknown);
      mock_stream_->PushRecvMessage(std::move(recv_msg_unknown));

      mock_stream_->Join();

      fiber_latch.CountDown();
    });

    fiber_latch.Wait();

    ASSERT_TRUE(mock_stream_->IsTerminate());
  });
}

TEST_F(CommonStreamTest, HandleUnknownSendMessageTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);
    SetStreamHandlerMockExpect();
    StartFiberDetached([&]() {
      EXPECT_CALL(*mock_stream_, SendReset_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kError));

      StreamSendMessage send_msg_unknown;
      send_msg_unknown.category = StreamMessageCategory::kStreamUnknown;
      mock_stream_->PushSendMessage(std::move(send_msg_unknown));

      mock_stream_->Join();

      fiber_latch.CountDown();
    });

    fiber_latch.Wait();

    ASSERT_TRUE(mock_stream_->IsTerminate());
  });
}

TEST_F(CommonStreamTest, StartOkTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(2);
    StartFiberDetached([&]() {
      Status status = mock_stream_->Start();
      ASSERT_TRUE(status.OK());
      fiber_latch.CountDown();
    });

    StartFiberDetached([&]() {
      mock_stream_->OnReady();
      fiber_latch.CountDown();
    });

    fiber_latch.Wait();
  });
}

TEST_F(CommonStreamTest, StartTimeoutTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);
    SetStreamHandlerMockExpect();
    StartFiberDetached([&]() {
      EXPECT_CALL(*mock_stream_, SendReset_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kError));
      Status status = mock_stream_->Start();
      ASSERT_FALSE(status.OK());

      mock_stream_->Join();

      fiber_latch.CountDown();
    });

    fiber_latch.Wait();

    ASSERT_TRUE(mock_stream_->IsTerminate());
  });
}

TEST_F(CommonStreamTest, ReadOkTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(2);

    const char msg_str[] = "hello stream";
    NoncontiguousBufferBuilder builder;
    builder.Append(msg_str, strlen(msg_str));
    NoncontiguousBuffer msg = builder.DestructiveGet();
    int read_timeout = 3000;  // 3000ms

    StartFiberDetached([&]() {
      NoncontiguousBuffer read_msg;
      Status status = mock_stream_->Read(&read_msg, read_timeout);
      ASSERT_TRUE(status.OK());
      ASSERT_EQ(read_msg.ByteSize(), strlen(msg_str));
      fiber_latch.CountDown();
    });

    StartFiberDetached([&]() {
      mock_stream_->OnData(std::move(msg));
      fiber_latch.CountDown();
    });

    fiber_latch.Wait();
  });
}

TEST_F(CommonStreamTest, ReadTimeoutTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);

    const char msg_str[] = "hello stream";
    NoncontiguousBufferBuilder builder;
    builder.Append(msg_str, strlen(msg_str));
    NoncontiguousBuffer msg = builder.DestructiveGet();
    int read_timeout = 1000;  // 1000ms

    StartFiberDetached([&]() {
      NoncontiguousBuffer read_msg;
      Status status = mock_stream_->Read(&read_msg, read_timeout);
      ASSERT_FALSE(status.OK());
      ASSERT_EQ(read_msg.ByteSize(), 0);
      fiber_latch.CountDown();
    });

    fiber_latch.Wait();
  });
}

TEST_F(CommonStreamTest, ReadErrorTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(2);

    int read_timeout = 3000;  // 3000ms
    int error_code = 23158;   // random error code for unit test

    StartFiberDetached([&]() {
      NoncontiguousBuffer read_msg;
      Status status = mock_stream_->Read(&read_msg, read_timeout);
      ASSERT_FALSE(status.OK());
      ASSERT_EQ(status.GetFrameworkRetCode(), error_code);
      ASSERT_EQ(read_msg.ByteSize(), 0);
      fiber_latch.CountDown();
    });

    StartFiberDetached([&]() {
      Status status{error_code, 0, "Error"};
      mock_stream_->OnError(status);
      fiber_latch.CountDown();
    });

    fiber_latch.Wait();
  });
}

TEST_F(CommonStreamTest, ReadEofTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(2);

    int read_timeout = 3000;  // 3000ms

    StartFiberDetached([&]() {
      NoncontiguousBuffer read_msg;
      Status status = mock_stream_->Read(&read_msg, read_timeout);
      ASSERT_TRUE(status.StreamEof());
      ASSERT_EQ(read_msg.ByteSize(), 0);
      fiber_latch.CountDown();
    });

    StartFiberDetached([&]() {
      mock_stream_->OnFinish(kDefaultStatus);
      fiber_latch.CountDown();
    });

    fiber_latch.Wait();
  });
}

TEST_F(CommonStreamTest, WriteOkTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);

    SetStreamHandlerMockExpect();
    StartFiberDetached([&]() {
      // In order to exit the stream coroutine processing and detect that SendData has been executed, return kError to
      // terminate the stream.
      EXPECT_CALL(*mock_stream_, SendData_rvr(::testing::_)).WillOnce(::testing::Return(MockRetCode::kError));
      Status status = mock_stream_->Write(NoncontiguousBuffer{});
      ASSERT_TRUE(status.OK());

      mock_stream_->Join();
      fiber_latch.CountDown();
    });

    fiber_latch.Wait();

    ASSERT_TRUE(mock_stream_->IsTerminate());
  });
}

TEST_F(CommonStreamTest, WriteFailedTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);
    SetStreamHandlerMockExpect();

    StartFiberDetached([&]() {
      EXPECT_CALL(*mock_stream_, SendData_rvr(::testing::_)).Times(0);

      Status error_status{23157, 0, "error"};
      mock_stream_->OnError(error_status);

      Status status = mock_stream_->Write(NoncontiguousBuffer{});
      ASSERT_FALSE(status.OK());
      ASSERT_EQ(status.GetFrameworkRetCode(), error_status.GetFrameworkRetCode());

      mock_stream_->Stop();
      mock_stream_->Join();
      fiber_latch.CountDown();
    });

    fiber_latch.Wait();
  });
}

TEST_F(CommonStreamTest, SendOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    ASSERT_TRUE(mock_stream_->Send(NoncontiguousBuffer{}));
  });
}

TEST_F(CommonStreamTest, SendFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

    ASSERT_FALSE(mock_stream_->Send(NoncontiguousBuffer{}));
    Status status = mock_stream_->GetStatus();
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), -1);
  });
}

class AsyncCommonStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    MergeWorkerThread::Options opt;
    mthread_ = std::make_unique<MergeWorkerThread>(std::move(opt));
    mthread_->Start();

    StreamOptions options;
    options.stream_id = 1;
    options.fiber_mode = false;
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;
    mock_stream_ = MakeRefCounted<MockCommonStream>(std::move(options));
  }

  void TearDown() override {
    mthread_->Stop();
    mthread_->Join();
    mthread_->Destroy();
    mthread_.reset();
  }

  void RunInMerge(std::function<void()>&& task) { EXPECT_TRUE(mthread_->GetReactor()->SubmitTask(std::move(task))); }

  void SetStreamHandlerMockExpect() { EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1); }

 protected:
  std::unique_ptr<MergeWorkerThread> mthread_;
  MockStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
};

TEST_F(AsyncCommonStreamTest, AsyncStart1) {
  Latch l(1);
  RunInMerge([&]() {
    mock_stream_->OnReady();
    EXPECT_TRUE(mock_stream_->AsyncStart().IsReady());
    l.count_down();
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncStart2) {
  Latch l(1);
  RunInMerge([&]() {
    mock_stream_->OnError(kUnknownErrorStatus);
    EXPECT_TRUE(mock_stream_->AsyncStart().IsFailed());
    l.count_down();
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncFinish1) {
  Latch l(1);
  RunInMerge([&]() {
    mock_stream_->OnFinish(kDefaultStatus);
    EXPECT_TRUE(mock_stream_->AsyncFinish().IsReady());
    l.count_down();
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncFinish2) {
  Latch l(1);
  RunInMerge([&]() {
    mock_stream_->OnError(kUnknownErrorStatus);
    EXPECT_TRUE(mock_stream_->AsyncFinish().IsReady());
    l.count_down();
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncRead1) {
  Latch l(1);
  RunInMerge([&]() {
    mock_stream_->OnData(NoncontiguousBuffer{});
    EXPECT_TRUE(mock_stream_->AsyncRead().IsReady());
    l.count_down();
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncRead2) {
  Latch l(1);
  RunInMerge([&]() {
    mock_stream_->OnFinish(kDefaultStatus);
    EXPECT_TRUE(mock_stream_->AsyncRead().IsReady());
    l.count_down();
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncRead3) {
  Latch l(1);
  RunInMerge([&]() {
    auto ft = mock_stream_->AsyncRead(1).Then([&](Future<std::optional<NoncontiguousBuffer>>&& ft) {
      EXPECT_TRUE(ft.IsFailed());
      Status expected(mock_stream_->GetReadTimeoutErrorCode(), 0, "read message timeout");
      EXPECT_EQ(ft.GetException().what(), expected.ToString());
      l.count_down();
      return MakeReadyFuture<>();
    });
    EXPECT_FALSE(ft.IsReady() || ft.IsFailed());
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncWrite1) {
  Latch l(1);
  RunInMerge([&]() {
    EXPECT_CALL(*mock_stream_, SendData_rvr(::testing::_)).WillOnce(::testing::Return(RetCode::kSuccess));
    EXPECT_TRUE(mock_stream_->AsyncWrite(NoncontiguousBuffer{}).IsReady());
    EXPECT_FALSE(mock_stream_->AsyncFinish().IsReady() || mock_stream_->AsyncFinish().IsFailed());
    l.count_down();
  });

  l.wait();
}

TEST_F(AsyncCommonStreamTest, AsyncWrite2) {
  Latch l(1);
  RunInMerge([&]() {
    SetStreamHandlerMockExpect();
    EXPECT_CALL(*mock_stream_, SendData_rvr(::testing::_)).WillOnce(::testing::Return(RetCode::kError));
    EXPECT_TRUE(mock_stream_->AsyncWrite(NoncontiguousBuffer{}).IsFailed());
    EXPECT_TRUE(mock_stream_->AsyncFinish().IsReady());
    l.count_down();
  });

  l.wait();
}

}  // namespace trpc::testing
