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

#include "trpc/stream/trpc/trpc_server_stream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_server_codec.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/server/server_context.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/stream/trpc/testing/trpc_stream_testing.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockTrpcServerStream : public TrpcServerStream {
 public:
  using MockRetCode = FiberStreamJobScheduler::RetCode;
  using MockAction = CommonStream::Action;
  using MockState = CommonStream::State;

  explicit MockTrpcServerStream(StreamOptions&& options) : TrpcServerStream(std::move(options)) {}

  RetCode HandleInit(StreamRecvMessage&& msg) override { return TrpcServerStream::HandleInit(std::move(msg)); }

  RetCode HandleData(StreamRecvMessage&& msg) override { return TrpcServerStream::HandleData(std::move(msg)); }

  RetCode HandleFeedback(StreamRecvMessage&& msg) override { return TrpcServerStream::HandleFeedback(std::move(msg)); }

  RetCode HandleClose(StreamRecvMessage&& msg) override { return TrpcServerStream::HandleClose(std::move(msg)); }

  RetCode HandleReset(StreamRecvMessage&& msg) override { return TrpcServerStream::HandleReset(std::move(msg)); }

  RetCode SendInit(StreamSendMessage&& msg) override { return TrpcServerStream::SendInit(std::move(msg)); }

  RetCode SendData(StreamSendMessage&& msg) override { return TrpcServerStream::SendData(std::move(msg)); }

  RetCode SendFeedback(StreamSendMessage&& msg) override { return TrpcServerStream::SendFeedback(std::move(msg)); }

  RetCode SendClose(StreamSendMessage&& msg) override { return TrpcServerStream::SendClose(std::move(msg)); }

  RetCode SendReset(StreamSendMessage&& msg) override { return TrpcStream::SendReset(std::move(msg)); }

  bool EncodeMessage(Protocol* protocol, NoncontiguousBuffer* buffer) {
    return TrpcStream::EncodeMessage(protocol, buffer);
  }

  bool DecodeMessage(NoncontiguousBuffer* buffer, Protocol* protocol) {
    return TrpcStream::DecodeMessage(buffer, protocol);
  }

  void SetState(State state) { TrpcServerStream::SetState(state); }
  State GetState() const { return TrpcServerStream::GetState(); }

  bool CheckState(State state, Action action) { return TrpcServerStream::CheckState(state, action); }

  RefPtr<TrpcStreamRecvController> GetRecvController() { return recv_flow_controller_; }

  RefPtr<TrpcStreamSendController> GetSendController() { return send_flow_controller_; }
  void SetSendController(RefPtr<TrpcStreamSendController> send_controller) { send_flow_controller_ = send_controller; }
};

using MockRetCode = MockTrpcServerStream::MockRetCode;
using MockAction = MockTrpcServerStream::MockAction;
using MockState = MockTrpcServerStream::MockState;
using MockStreamPtr = RefPtr<MockTrpcServerStream>;

class TrpcServerStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StreamOptions options;
    options.stream_id = 1;
    auto svr_ctx = MakeRefCounted<ServerContext>();
    TrpcServerCodec codec;
    svr_ctx->SetRequestMsg(codec.CreateRequestObject());
    svr_ctx->SetResponseMsg(codec.CreateResponseObject());
    options.context.context = svr_ctx;
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;
    options.server_mode = true;
    options.fiber_mode = true;
    mock_stream_ = MakeRefCounted<MockTrpcServerStream>(std::move(options));
  }

  void TearDown() override {}

  void MockOnInitStreamingRpcCallback(int rc) {
    mock_stream_->GetMutableStreamOptions()->callbacks.handle_streaming_rpc_cb = [rc](STransportReqMsg* recv) {
      ServerContextPtr context = std::any_cast<ServerContextPtr&&>(std::move(recv->context));
      if (rc != 0) {
        context->SetStatus(Status{rc, "internal error"});
      }
    };
  }

 protected:
  MockStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
};

TEST_F(TrpcServerStreamTest, CheckStateOkTest) {
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kIdle, MockAction::kHandleInit));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kInit, MockAction::kSendInit));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kHandleData));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kLocalClosed, MockAction::kHandleData));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kSendData));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kRemoteClosed, MockAction::kSendData));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kHandleClose));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kRemoteClosed, MockAction::kSendClose));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kSendClose));
}

TEST_F(TrpcServerStreamTest, CheckStateFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    ASSERT_FALSE(mock_stream_->CheckState(MockState::kIdle, MockAction::kSendFeedback));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, GetErrorCodeTest) {
  ASSERT_EQ(mock_stream_->GetEncodeErrorCode(), TrpcRetCode::TRPC_STREAM_SERVER_ENCODE_ERR);
  ASSERT_EQ(mock_stream_->GetDecodeErrorCode(), TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR);
  ASSERT_EQ(mock_stream_->GetReadTimeoutErrorCode(), TrpcRetCode::TRPC_STREAM_SERVER_READ_TIMEOUT_ERR);
}

TEST_F(TrpcServerStreamTest, SendRecvNotImplementTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);

    ASSERT_EQ(mock_stream_->HandleReset(StreamRecvMessage{}), MockRetCode::kError);
    ASSERT_EQ(mock_stream_->SendReset(StreamSendMessage{}), MockRetCode::kError);
  });
}

TEST_F(TrpcServerStreamTest, CloseTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    StartFiberDetached([&]() {
      EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

      mock_stream_->Close(kDefaultStatus);

      mock_stream_->Join();

      ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);

      fiber_latch.CountDown();
    });

    fiber_latch.Wait();

    ASSERT_TRUE(mock_stream_->IsTerminate());
  });
}

TEST_F(TrpcServerStreamTest, ResetTest) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    StartFiberDetached([&]() {
      EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

      mock_stream_->Reset(kDefaultStatus);

      mock_stream_->Join();

      ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);

      fiber_latch.CountDown();
    });

    fiber_latch.Wait();

    ASSERT_TRUE(mock_stream_->IsTerminate());
  });
}

TEST_F(TrpcServerStreamTest, HandleInitOkTest) {
  RunAsFiber([&]() {
    MockOnInitStreamingRpcCallback(0);
    auto init_frame = CreateTrpcStreamInitFrameProtocol(mock_stream_->GetId(), TrpcStreamInitMeta{});
    NoncontiguousBuffer init_buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&init_frame, &init_buffer));
    StreamRecvMessage recv_msg;
    recv_msg.message = init_buffer;

    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kInit);

    // When Init is received, respond with an Init frame to the peer (asynchronous sending, need to wait for a while).
    ::trpc::FiberSleepFor(std::chrono::seconds(3));

    ASSERT_TRUE(mock_stream_->GetRecvController() == nullptr);
    ASSERT_TRUE(mock_stream_->GetSendController() == nullptr);
  });
}

TEST_F(TrpcServerStreamTest, HandleInitFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR);
  });
}

TEST_F(TrpcServerStreamTest, HandleInitFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(MockRetCode::kError, mock_stream_->HandleInit(StreamRecvMessage{}));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, HandleDataOkTest) {
  RunAsFiber([&]() {
    auto frame = CreateTrpcStreamDataFrameProtocol(mock_stream_->GetId(), NoncontiguousBuffer{});
    NoncontiguousBuffer buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&frame, &buffer));

    StreamRecvMessage recv_msg;
    recv_msg.message = buffer;

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleData(std::move(recv_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(TrpcServerStreamTest, HandleDataFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleData(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR);
  });
}

TEST_F(TrpcServerStreamTest, HandleDataFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(MockRetCode::kError, mock_stream_->HandleData(StreamRecvMessage{}));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, HandleCloseOkTest) {
  RunAsFiber([&]() {
    TrpcStreamCloseMeta meta;
    meta.set_close_type(TrpcStreamCloseType::TRPC_STREAM_CLOSE);

    auto frame = CreateTrpcStreamCloseFrameProtocol(mock_stream_->GetId(), TrpcStreamCloseMeta{});
    NoncontiguousBuffer buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&frame, &buffer));

    StreamRecvMessage recv_msg;
    recv_msg.message = buffer;

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kRemoteClosed);
  });
}

TEST_F(TrpcServerStreamTest, HandleCloseResetTest) {
  RunAsFiber([&]() {
    TrpcStreamCloseMeta meta;
    meta.set_ret(TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
    meta.set_close_type(TrpcStreamCloseType::TRPC_STREAM_RESET);

    auto frame = CreateTrpcStreamCloseFrameProtocol(mock_stream_->GetId(), meta);
    NoncontiguousBuffer buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&frame, &buffer));

    StreamRecvMessage recv_msg;
    recv_msg.message = buffer;

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, HandleCloseInClosedStateTest) {
  RunAsFiber([&]() {
    StreamRecvMessage recv_msg;

    mock_stream_->SetState(MockState::kClosed);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(TrpcServerStreamTest, HandleCloseFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR);
  });
}

TEST_F(TrpcServerStreamTest, HandleCloseFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    TrpcStreamCloseMeta meta;
    meta.set_close_type(TrpcStreamCloseType::TRPC_STREAM_CLOSE);

    auto frame = CreateTrpcStreamCloseFrameProtocol(mock_stream_->GetId(), TrpcStreamCloseMeta{});
    NoncontiguousBuffer buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&frame, &buffer));

    StreamRecvMessage recv_msg;
    recv_msg.message = buffer;

    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, SendInitOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockOnInitStreamingRpcCallback(0);

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kOpen);
  });
}

TEST_F(TrpcServerStreamTest, SendInitFailedDueToStateErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, SendInitFailedDueToInitStreamingRpcErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    int error_code = 17458;
    MockOnInitStreamingRpcCallback(error_code);  // set random error code

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
  });
}

TEST_F(TrpcServerStreamTest, SendDataOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.data_provider = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendData(std::move(send_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(TrpcServerStreamTest, SendDataFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendData(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, SendCloseOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.metadata = TrpcStreamCloseMeta{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendClose(std::move(send_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
  });
}

TEST_F(TrpcServerStreamTest, SendCloseFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendClose(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcServerStreamTest, HandleFeedbackOKTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);

    auto init_frame = CreateTrpcStreamFeedbackFrameProtocol(mock_stream_->GetId(), TrpcStreamFeedBackMeta{});
    NoncontiguousBuffer init_buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&init_frame, &init_buffer));
    StreamRecvMessage recv_msg;
    recv_msg.message = init_buffer;

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleFeedback(std::move(recv_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(TrpcServerStreamTest, HandleFeedbackFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleFeedback(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR);
  });
}

TEST_F(TrpcServerStreamTest, TestFlowControllerAssign) {
  RunAsFiber([&]() {
    MockOnInitStreamingRpcCallback(0);
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    TrpcStreamInitMeta meta;
    meta.set_init_window_size(100);
    auto init_frame = CreateTrpcStreamInitFrameProtocol(mock_stream_->GetId(), std::move(meta));
    NoncontiguousBuffer init_buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&init_frame, &init_buffer));
    StreamRecvMessage recv_msg;
    recv_msg.message = init_buffer;

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kSuccess);

    // When Init is received, respond with an Init frame to the peer (asynchronous sending, need to wait for a while).
    ::trpc::FiberSleepFor(std::chrono::seconds(3));

    ASSERT_TRUE(mock_stream_->GetRecvController() != nullptr);
    ASSERT_TRUE(mock_stream_->GetSendController() != nullptr);
  });
}

}  // namespace trpc::testing
