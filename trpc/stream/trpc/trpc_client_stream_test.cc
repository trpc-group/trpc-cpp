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

#include "trpc/stream/trpc/trpc_client_stream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/stream/trpc/testing/trpc_stream_testing.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockTrpcClientStream : public TrpcClientStream {
 public:
  using MockRetCode = FiberStreamJobScheduler::RetCode;
  using MockAction = CommonStream::Action;
  using MockState = CommonStream::State;

  explicit MockTrpcClientStream(StreamOptions&& options) : TrpcClientStream(std::move(options)) {}

  RetCode HandleInit(StreamRecvMessage&& msg) override { return TrpcClientStream::HandleInit(std::move(msg)); }

  RetCode HandleData(StreamRecvMessage&& msg) override { return TrpcClientStream::HandleData(std::move(msg)); }

  RetCode HandleFeedback(StreamRecvMessage&& msg) override { return TrpcClientStream::HandleFeedback(std::move(msg)); }

  RetCode HandleClose(StreamRecvMessage&& msg) override { return TrpcClientStream::HandleClose(std::move(msg)); }

  RetCode HandleReset(StreamRecvMessage&& msg) override { return TrpcClientStream::HandleReset(std::move(msg)); }

  RetCode SendInit(StreamSendMessage&& msg) override { return TrpcClientStream::SendInit(std::move(msg)); }

  RetCode SendData(StreamSendMessage&& msg) override { return TrpcClientStream::SendData(std::move(msg)); }

  RetCode SendFeedback(StreamSendMessage&& msg) override { return TrpcClientStream::SendFeedback(std::move(msg)); }

  RetCode SendClose(StreamSendMessage&& msg) override { return TrpcClientStream::SendClose(std::move(msg)); }

  RetCode SendReset(StreamSendMessage&& msg) override { return TrpcStream::SendReset(std::move(msg)); }

  void PushRecvMessage(StreamRecvMessage&& msg) override { ++recv_size_; }

  RetCode PushSendMessage(StreamSendMessage&& msg, bool push_front = false) override {
    ++send_size_;
    return RetCode::kSuccess;
  }

  void OnReady() { CommonStream::OnReady(); }

  void OnData(NoncontiguousBuffer&& msg) { CommonStream::OnData(std::move(msg)); }

  void OnError(Status status) { CommonStream::OnError(status); }

  bool EncodeMessage(Protocol* protocol, NoncontiguousBuffer* buffer) {
    return TrpcStream::EncodeMessage(protocol, buffer);
  }

  bool DecodeMessage(NoncontiguousBuffer* buffer, Protocol* protocol) {
    return TrpcStream::DecodeMessage(buffer, protocol);
  }

  State GetState() const { return TrpcClientStream::GetState(); }

  bool CheckState(State state, Action action) { return TrpcClientStream::CheckState(state, action); }

  void SetState(State state) { TrpcClientStream::SetState(state); }

  std::size_t GetRecvQueueSize() const { return recv_size_; }

  std::size_t GetSendQueueSize() const { return send_size_; }

  RefPtr<TrpcStreamRecvController> GetRecvController() { return recv_flow_controller_; }

  RefPtr<TrpcStreamSendController> GetSendController() { return send_flow_controller_; }

 private:
  std::size_t recv_size_{0};

  std::size_t send_size_{0};
};

class MockSerialization : public serialization::Serialization {
 public:
  MockSerialization() = default;

  ~MockSerialization() override = default;

  serialization::SerializationType Type() const override { return 129; }

  MOCK_METHOD(bool, Serialize, (serialization::DataType, void*, NoncontiguousBuffer*), (override));

  MOCK_METHOD(bool, Deserialize, (NoncontiguousBuffer*, serialization::DataType, void*), (override));
};

using MockRetCode = MockTrpcClientStream::MockRetCode;
using MockAction = MockTrpcClientStream::MockAction;
using MockState = MockTrpcClientStream::MockState;
using MockStreamPtr = RefPtr<MockTrpcClientStream>;
using MockSerializationPtr = RefPtr<MockSerialization>;

class TrpcClientStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto ctx = MakeRefCounted<ClientContext>();
    ctx->SetRequest(std::make_shared<TrpcRequestProtocol>());

    StreamOptions options;
    options.stream_id = 1;
    options.context.context = std::move(ctx);
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;
    options.server_mode = false;
    options.fiber_mode = true;
    mock_stream_ = MakeRefCounted<MockTrpcClientStream>(std::move(options));
    mock_serialization_ = MakeRefCounted<MockSerialization>();
    serialization::SerializationFactory::GetInstance()->Register(mock_serialization_);
  }

  void TearDown() override { serialization::SerializationFactory::GetInstance()->Clear(); }

  void MockRpcReplyMsgDoingSuccess() {
    ClientContextPtr context =
        std::any_cast<ClientContextPtr>(mock_stream_->GetMutableStreamOptions()->context.context);
    context->SetReqEncodeType(mock_serialization_->Type());

    int* reply_msg = new int();
    mock_stream_->GetMutableStreamOptions()->rpc_reply_msg = reply_msg;
    EXPECT_CALL(*mock_serialization_, Deserialize(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(true));
  }

  void MockRpcReplyMsgDoingFailedDueToDeserializeError() {
    ClientContextPtr context =
        std::any_cast<ClientContextPtr>(mock_stream_->GetMutableStreamOptions()->context.context);
    context->SetReqEncodeType(mock_serialization_->Type());

    int* reply_msg = new int();
    mock_stream_->GetMutableStreamOptions()->rpc_reply_msg = reply_msg;
    EXPECT_CALL(*mock_serialization_, Deserialize(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(false));
  }

 protected:
  MockStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
  MockSerializationPtr mock_serialization_{nullptr};
};

TEST_F(TrpcClientStreamTest, CheckStateOkTest) {
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kInit, MockAction::kHandleInit));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kIdle, MockAction::kSendInit));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kHandleData));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kLocalClosed, MockAction::kHandleData));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kSendData));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kHandleClose));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kLocalClosed, MockAction::kHandleClose));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kSendClose));
}

TEST_F(TrpcClientStreamTest, CheckStateFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    ASSERT_FALSE(mock_stream_->CheckState(MockState::kIdle, MockAction::kSendFeedback));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, GetErrorCodeTest) {
  ASSERT_EQ(mock_stream_->GetEncodeErrorCode(), TrpcRetCode::TRPC_STREAM_CLIENT_ENCODE_ERR);
  ASSERT_EQ(mock_stream_->GetDecodeErrorCode(), TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR);
  ASSERT_EQ(mock_stream_->GetReadTimeoutErrorCode(), TrpcRetCode::TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
}

TEST_F(TrpcClientStreamTest, SendRecvNotImplementTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);

    ASSERT_EQ(mock_stream_->HandleReset(StreamRecvMessage{}), MockRetCode::kError);
    ASSERT_EQ(mock_stream_->SendReset(StreamSendMessage{}), MockRetCode::kError);
  });
}

TEST_F(TrpcClientStreamTest, HandleInitOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);

    auto init_frame = CreateTrpcStreamInitFrameProtocol(mock_stream_->GetId(), TrpcStreamInitMeta{});
    NoncontiguousBuffer init_buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&init_frame, &init_buffer));
    StreamRecvMessage recv_msg;
    recv_msg.message = init_buffer;

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kOpen);

    ASSERT_TRUE(mock_stream_->GetRecvController() == nullptr);
    ASSERT_TRUE(mock_stream_->GetSendController() == nullptr);
  });
}

TEST_F(TrpcClientStreamTest, HandleInitFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR);
  });
}

TEST_F(TrpcClientStreamTest, HandleInitFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(MockRetCode::kError, mock_stream_->HandleInit(StreamRecvMessage{}));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, HandleInitFailedDueToInitRspErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);

    TrpcStreamInitMeta meta;
    meta.mutable_response_meta()->set_ret(TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);

    auto init_frame = CreateTrpcStreamInitFrameProtocol(mock_stream_->GetId(), meta);
    NoncontiguousBuffer init_buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&init_frame, &init_buffer));
    StreamRecvMessage recv_msg;
    recv_msg.message = init_buffer;

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, HandleDataOkTest) {
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

TEST_F(TrpcClientStreamTest, HandleDataFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleData(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR);
  });
}

TEST_F(TrpcClientStreamTest, HandleDataFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(MockRetCode::kError, mock_stream_->HandleData(StreamRecvMessage{}));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, HandleCloseOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));

    TrpcStreamCloseMeta meta;
    meta.set_close_type(TrpcStreamCloseType::TRPC_STREAM_CLOSE);

    auto frame = CreateTrpcStreamCloseFrameProtocol(mock_stream_->GetId(), TrpcStreamCloseMeta{});
    NoncontiguousBuffer buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&frame, &buffer));

    StreamRecvMessage recv_msg;
    recv_msg.message = buffer;

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
  });
}

TEST_F(TrpcClientStreamTest, HandleCloseResetTest) {
  RunAsFiber([&]() {
    TrpcStreamCloseMeta meta;
    meta.set_ret(TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
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
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, HandleCloseInClosedStateTest) {
  RunAsFiber([&]() {
    StreamRecvMessage recv_msg;

    mock_stream_->SetState(MockState::kClosed);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(TrpcClientStreamTest, HandleCloseFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR);
  });
}

TEST_F(TrpcClientStreamTest, HandleCloseFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    TrpcStreamCloseMeta meta;
    meta.set_close_type(TrpcStreamCloseType::TRPC_STREAM_CLOSE);

    auto frame = CreateTrpcStreamCloseFrameProtocol(mock_stream_->GetId(), TrpcStreamCloseMeta{});
    NoncontiguousBuffer buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&frame, &buffer));

    StreamRecvMessage recv_msg;
    recv_msg.message = buffer;

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, SendInitOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kInit);
  });
}

TEST_F(TrpcClientStreamTest, SendInitFailedDueToStateErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, SendDataOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.data_provider = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendData(std::move(send_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(TrpcClientStreamTest, SendDataFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendData(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, SendCloseOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.metadata = TrpcStreamCloseMeta{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendClose(std::move(send_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kLocalClosed);
  });
}

TEST_F(TrpcClientStreamTest, SendCloseFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendClose(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
  });
}

TEST_F(TrpcClientStreamTest, StartOkTest) {
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

    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 1);
  });
}

TEST_F(TrpcClientStreamTest, WriteDoneOkWithoutReplyTest) {
  RunAsFiber([&]() {
    Status status = mock_stream_->WriteDone();
    ASSERT_TRUE(status.OK());
    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 1);
  });
}

TEST_F(TrpcClientStreamTest, WriteDoneFailedDueToStatusErrorTest) {
  RunAsFiber([&]() {
    Status status{TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, 0, "error"};
    mock_stream_->OnError(status);

    Status write_status = mock_stream_->WriteDone();
    ASSERT_FALSE(write_status.OK());
    ASSERT_EQ(write_status.GetFrameworkRetCode(), status.GetFrameworkRetCode());
    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 0);
  });
}

TEST_F(TrpcClientStreamTest, WriteDoneSuccessWithReplyTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockRpcReplyMsgDoingSuccess();

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kSuccess);
    ASSERT_EQ(mock_stream_->GetState(), MockState::kInit);

    mock_stream_->OnData(NoncontiguousBuffer{});

    Status status = mock_stream_->WriteDone();

    ASSERT_TRUE(status.OK());
    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 1);
  });
}

TEST_F(TrpcClientStreamTest, WriteDoneFailedToDeserializeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    MockRpcReplyMsgDoingFailedDueToDeserializeError();

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kSuccess);
    ASSERT_EQ(mock_stream_->GetState(), MockState::kInit);

    mock_stream_->OnData(NoncontiguousBuffer{});

    Status status = mock_stream_->WriteDone();

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR);
    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 1);
  });
}

TEST_F(TrpcClientStreamTest, HandleFeedbackOKTest) {
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

TEST_F(TrpcClientStreamTest, HandleFeedbackFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamRecvMessage recv_msg;
    recv_msg.message = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleFeedback(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR);
  });
}

TEST_F(TrpcClientStreamTest, TestFlowControllerAssign) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);

    TrpcStreamInitMeta meta;
    meta.set_init_window_size(100);
    auto init_frame = CreateTrpcStreamInitFrameProtocol(mock_stream_->GetId(), std::move(meta));
    NoncontiguousBuffer init_buffer;
    ASSERT_TRUE(mock_stream_->EncodeMessage(&init_frame, &init_buffer));
    StreamRecvMessage recv_msg;
    recv_msg.message = init_buffer;

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kSuccess);

    ASSERT_TRUE(mock_stream_->GetRecvController() != nullptr);
    ASSERT_TRUE(mock_stream_->GetSendController() != nullptr);
  });
}

}  // namespace trpc::testing
