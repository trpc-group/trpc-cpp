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

#include "trpc/stream/grpc/grpc_server_stream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/grpc_stream_frame.h"
#include "trpc/codec/grpc/http2/testing/mock_session.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/server/rpc_service_impl.h"
#include "trpc/server/server_context.h"
#include "trpc/server/stream_rpc_method_handler.h"
#include "trpc/stream/stream.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

class MockGrpcServerStream : public GrpcServerStream {
 public:
  using MockState = GrpcServerStream::State;
  using MockRetCode = FiberStreamJobScheduler::RetCode;
  using MockAction = CommonStream::Action;
  using MockEncodeMessageType = GrpcServerStream::EncodeMessageType;

  explicit MockGrpcServerStream(StreamOptions&& options, http2::Session* session, FiberMutex* mutex,
                                FiberConditionVariable* cv)
      : GrpcServerStream(std::move(options), session, mutex, cv) {}

  RetCode HandleInit(StreamRecvMessage&& msg) override { return GrpcServerStream::HandleInit(std::move(msg)); }

  RetCode HandleData(StreamRecvMessage&& msg) override { return GrpcServerStream::HandleData(std::move(msg)); }

  RetCode HandleFeedback(StreamRecvMessage&& msg) override { return GrpcServerStream::HandleFeedback(std::move(msg)); }

  RetCode HandleClose(StreamRecvMessage&& msg) override { return GrpcServerStream::HandleClose(std::move(msg)); }

  RetCode HandleReset(StreamRecvMessage&& msg) override { return GrpcServerStream::HandleReset(std::move(msg)); }

  RetCode SendInit(StreamSendMessage&& msg) override { return GrpcServerStream::SendInit(std::move(msg)); }

  RetCode SendData(StreamSendMessage&& msg) override { return GrpcServerStream::SendData(std::move(msg)); }

  RetCode SendFeedback(StreamSendMessage&& msg) override { return GrpcServerStream::SendFeedback(std::move(msg)); }

  RetCode SendClose(StreamSendMessage&& msg) override { return GrpcServerStream::SendClose(std::move(msg)); }

  RetCode SendReset(StreamSendMessage&& msg) override { return GrpcStream::SendReset(std::move(msg)); }

  void PushRecvMessage(StreamRecvMessage&& msg) override { ++recv_size_; }

  RetCode PushSendMessage(StreamSendMessage&& msg, bool push_front = false) override {
    ++send_size_;
    return RetCode::kSuccess;
  }

  void SetState(State state) { GrpcServerStream::SetState(state); }
  State GetState() const { return GrpcServerStream::GetState(); }

  bool CheckState(State state, Action action) { return GrpcServerStream::CheckState(state, action); }

  std::size_t GetRecvQueueSize() const { return recv_size_; }

  std::size_t GetSendQueueSize() const { return send_size_; }

 private:
  std::size_t recv_size_{0};

  std::size_t send_size_{0};
};

using MockRetCode = MockGrpcServerStream::MockRetCode;
using MockAction = MockGrpcServerStream::MockAction;
using MockState = MockGrpcServerStream::MockState;
using MockStreamPtr = RefPtr<MockGrpcServerStream>;
using MockEncodeMessageType = MockGrpcServerStream::MockEncodeMessageType;

class GrpcServerStreamTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { trpc::codec::Init(); }

  static void TearDownTestSuite() { trpc::codec::Destroy(); }

  void SetUp() override {
    StreamOptions options;
    options.server_mode = true;
    options.fiber_mode = true;
    options.stream_id = 1;

    ServerContextPtr context = MakeRefCounted<ServerContext>();
    server_codec_ = ServerCodecFactory::GetInstance()->Get("grpc");
    context->SetRequestMsg(server_codec_->CreateRequestObject());
    context->SetResponseMsg(server_codec_->CreateResponseObject());
    RpcServiceImpl* rpc_service = new RpcServiceImpl();
    context->SetService(rpc_service);
    context->GetService()->AddRpcServiceMethod(
        new trpc::RpcServiceMethod("/trpc.test.helloworld.Greeter/SayHello", trpc::MethodType::BIDI_STREAMING,
                                   new trpc::StreamRpcMethodHandler<std::string, std::string>(
                                       std::bind(&GrpcServerStreamTest::BidiStreamSayHello, this, std::placeholders::_1,
                                                 std::placeholders::_2, std::placeholders::_3))));

    options.context.context = context;

    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;

    mock_session_ = CreateMockHttp2Session(http2::SessionOptions());

    mock_stream_ = MakeRefCounted<MockGrpcServerStream>(std::move(options), mock_session_.get(), &mutex_, &cv_);

    EXPECT_CALL(*mock_session_, SignalWrite(::testing::_)).WillRepeatedly(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitReset(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(0));
  }

  trpc::Status BidiStreamSayHello(const trpc::ServerContextPtr& context,
                                  const trpc::stream::StreamReader<std::string>& reader,
                                  trpc::stream::StreamWriter<std::string>* writer) {
    return kDefaultStatus;
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

  void MockNotSupportedCompressType() {
    ServerContextPtr context =
        std::any_cast<ServerContextPtr>(mock_stream_->GetMutableStreamOptions()->context.context);
    context->SetRspCompressType(TrpcCompressType::TRPC_GZIP_COMPRESS);
  }

 protected:
  MockStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
  MockHttp2SessionPtr mock_session_{nullptr};
  FiberMutex mutex_;
  FiberConditionVariable cv_;
  ServerCodecPtr server_codec_;
};

TEST_F(GrpcServerStreamTest, CheckStateOkTest) {
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kIdle, MockAction::kHandleInit));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kHandleData));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kHandleClose));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kSendInit));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kRemoteClosed, MockAction::kSendInit));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kSendData));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kRemoteClosed, MockAction::kSendData));

  ASSERT_TRUE(mock_stream_->CheckState(MockState::kOpen, MockAction::kSendClose));
  ASSERT_TRUE(mock_stream_->CheckState(MockState::kRemoteClosed, MockAction::kSendClose));
}

TEST_F(GrpcServerStreamTest, CheckStateFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    ASSERT_FALSE(mock_stream_->CheckState(MockState::kIdle, MockAction::kSendFeedback));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendRecvNotImplementTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).Times(0);

    ASSERT_EQ(mock_stream_->HandleFeedback(StreamRecvMessage{}), MockRetCode::kError);
    ASSERT_EQ(mock_stream_->SendFeedback(StreamSendMessage{}), MockRetCode::kError);
  });
}

namespace {
http2::RequestPtr MakeHttp2Request() {
  http2::RequestPtr http2_request = http2::CreateRequest();
  http2_request->SetMethod("POST");
  http2_request->SetPath("/trpc.test.helloworld.Greeter/SayHello");
  http2_request->SetScheme("http");
  http2_request->SetAuthority("www.example.com");
  http2_request->AddHeader(kGrpcContentTypeName, kGrpcContentTypeDefault);
  http2_request->AddHeader(kGrpcEncodingName, kGrpcContentEncodingGzip);
  http2_request->AddHeader(kGrpcTimeoutName, "1M");
  http2_request->AddHeader("te", "trailers");
  http2_request->AddHeader("x-user-defined-01", "Tom");
  http2_request->AddHeader("x-user-defined-02", "Jerry");

  return http2_request;
}
}  // namespace

TEST_F(GrpcServerStreamTest, HandleInitOkTest) {
  RunAsFiber([&]() {
    MockOnInitStreamingRpcCallback(0);

    uint32_t stream_id = 1;
    http2::RequestPtr http2_request = MakeHttp2Request();
    GrpcStreamFramePtr frame = MakeRefCounted<GrpcStreamInitFrame>(stream_id, http2_request);

    StreamRecvMessage recv_msg;
    recv_msg.message = frame;

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kOpen);
  });
}

TEST_F(GrpcServerStreamTest, HandleInitFailedDueToInitStreamingRpcErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    int error_code = 17458;
    MockOnInitStreamingRpcCallback(error_code);  // set random error code

    uint32_t stream_id = 1;
    http2::RequestPtr http2_request = MakeHttp2Request();
    GrpcStreamFramePtr frame = MakeRefCounted<GrpcStreamInitFrame>(stream_id, http2_request);

    StreamRecvMessage recv_msg;
    recv_msg.message = frame;

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
  });
}

TEST_F(GrpcServerStreamTest, HandleInitFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(MockRetCode::kError, mock_stream_->HandleInit(StreamRecvMessage{}));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, HandleDataOkTest) {
  RunAsFiber([&]() {
    uint32_t stream_id = 1;
    NoncontiguousBuffer data;
    GrpcMessageContent grpc_message;
    ASSERT_TRUE(grpc_message.Encode(&data));

    GrpcStreamFramePtr frame = MakeRefCounted<GrpcStreamDataFrame>(stream_id, std::move(data));

    StreamRecvMessage recv_msg;
    recv_msg.message = frame;

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleData(std::move(recv_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(GrpcServerStreamTest, HandleDataFailedDueToDecodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    uint32_t stream_id = 1;
    NoncontiguousBuffer data;
    GrpcStreamFramePtr frame = MakeRefCounted<GrpcStreamDataFrame>(stream_id, std::move(data));

    StreamRecvMessage recv_msg;
    recv_msg.message = frame;

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->HandleData(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, HandleDataFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(MockRetCode::kError, mock_stream_->HandleData(StreamRecvMessage{}));

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, HandleCloseOkTest) {
  RunAsFiber([&]() {
    MockOnInitStreamingRpcCallback(0);
    uint32_t stream_id = 1;
    http2::RequestPtr http2_request = MakeHttp2Request();
    GrpcStreamFramePtr frame = MakeRefCounted<GrpcStreamInitFrame>(stream_id, http2_request);
    StreamRecvMessage recv_msg;
    recv_msg.message = frame;
    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kSuccess);
    ASSERT_EQ(mock_stream_->GetState(), MockState::kOpen);

    ASSERT_EQ(mock_stream_->HandleClose(StreamRecvMessage{}), MockRetCode::kSuccess);
    ASSERT_EQ(mock_stream_->GetState(), MockState::kRemoteClosed);
  });
}

TEST_F(GrpcServerStreamTest, HandleCloseInClosedStateTest) {
  RunAsFiber([&]() {
    mock_stream_->SetState(MockState::kClosed);
    ASSERT_EQ(mock_stream_->HandleClose(StreamRecvMessage{}), MockRetCode::kSuccess);
  });
}

TEST_F(GrpcServerStreamTest, HandleCloseFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    MockOnInitStreamingRpcCallback(0);
    uint32_t stream_id = 1;
    http2::RequestPtr http2_request = MakeHttp2Request();
    GrpcStreamFramePtr frame = MakeRefCounted<GrpcStreamInitFrame>(stream_id, http2_request);
    StreamRecvMessage recv_msg;
    recv_msg.message = frame;
    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->HandleInit(std::move(recv_msg)), MockRetCode::kSuccess);
    ASSERT_EQ(mock_stream_->GetState(), MockState::kOpen);

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->HandleClose(std::move(recv_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendInitOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kSuccess);
  });
}

TEST_F(GrpcServerStreamTest, SendInitFailedDueToEncodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendInitFailedDueToStateErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kInit);
    ASSERT_EQ(mock_stream_->SendInit(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendDataOkTest) {
  RunAsFiber([&]() {
    // Send INIT firstly, then send DATA.
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitData_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(0))
        .WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.data_provider = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendData(std::move(send_msg)), MockRetCode::kSuccess);
  });
}

TEST_F(GrpcServerStreamTest, SendDataFailedDueToBadStateTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendData(StreamSendMessage{}), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendDataFailedDueToSendInitFailedTest) {
  RunAsFiber([&]() {
    MockNotSupportedCompressType();

    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
    EXPECT_CALL(*mock_session_, SubmitData_rvr(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.data_provider = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendData(std::move(send_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendDataFailedDueToEncodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitData_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
    // Send RESET and HEADER.
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(0))
        .WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.data_provider = NoncontiguousBuffer{};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendData(std::move(send_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendCloseOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitTrailer(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).WillOnce(::testing::Return(0));
    // Send INIT and CLOSE.
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(0))
        .WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.metadata = Status{GrpcStatus::kGrpcInternal, 0, "test"};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendClose(std::move(send_msg)), MockRetCode::kSuccess);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
  });
}

TEST_F(GrpcServerStreamTest, SendCloseFailedDueToSendInitFailedTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
    EXPECT_CALL(*mock_session_, SubmitTrailer(::testing::_, ::testing::_)).Times(0);
    // Send RESET.
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.metadata = Status{GrpcStatus::kGrpcInternal, 0, "test"};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendClose(std::move(send_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendCloseFailedDueToEncodeErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
    EXPECT_CALL(*mock_session_, SubmitTrailer(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
    // Send INIT and RESET.
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(0))
        .WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.metadata = Status{GrpcStatus::kGrpcInternal, 0, "test"};

    mock_stream_->SetState(MockState::kOpen);
    ASSERT_EQ(mock_stream_->SendClose(std::move(send_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, SendCloseFailedDueToStateErrorTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, SubmitHeader(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mock_session_, SubmitTrailer(::testing::_, ::testing::_)).Times(0);
    // Send RESET.
    EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));

    StreamSendMessage send_msg;
    send_msg.metadata = Status{GrpcStatus::kGrpcInternal, 0, "test"};

    mock_stream_->SetState(MockState::kIdle);
    ASSERT_EQ(mock_stream_->SendClose(std::move(send_msg)), MockRetCode::kError);

    ASSERT_EQ(mock_stream_->GetState(), MockState::kClosed);
    Status status = mock_stream_->GetStatus();
    ASSERT_EQ(status.GetFrameworkRetCode(), GrpcStatus::kGrpcInternal);
  });
}

TEST_F(GrpcServerStreamTest, WriteOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, DecreaseRemoteWindowSize(::testing::_)).WillOnce(::testing::Return(true));

    Status status = mock_stream_->Write(NoncontiguousBuffer{});
    ASSERT_TRUE(status.OK());
    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 1);
  });
}

TEST_F(GrpcServerStreamTest, WriteOkWithFlowControllTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, DecreaseRemoteWindowSize(::testing::_))
        .WillOnce(::testing::Return(false))
        .WillOnce(::testing::Return(true));

    Status status = mock_stream_->Write(NoncontiguousBuffer{});
    ASSERT_TRUE(status.OK());
    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 1);
  });
}

TEST_F(GrpcServerStreamTest, WriteNotOkTest) {
  RunAsFiber([&]() {
    EXPECT_CALL(*mock_session_, DecreaseRemoteWindowSize(::testing::_)).Times(0);
    Status error_status{GrpcStatus::kGrpcInternal, 0, "error"};
    mock_stream_->OnError(error_status);
    Status status = mock_stream_->Write(NoncontiguousBuffer{});
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(error_status.GetFrameworkRetCode(), status.GetFrameworkRetCode());
    ASSERT_EQ(mock_stream_->GetSendQueueSize(), 0);
  });
}

}  // namespace trpc::testing
