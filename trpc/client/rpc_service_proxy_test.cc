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

#include "trpc/client/rpc_service_proxy.h"

#include <memory>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/client/testing/service_proxy_testing.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/future/future_utility.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/proto/testing/helloworld_generated.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/stream/testing/mock_stream_provider.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

namespace trpc::testing {

class MockTrpcClientCodec : public TrpcClientCodec {
 public:
  std::string Name() const { return "mock_codec"; }

  MOCK_METHOD(bool, FillRequest, (const ClientContextPtr& context, const ProtocolPtr& in, void* out), (override));
  MOCK_METHOD(bool, FillResponse, (const ClientContextPtr& context, const ProtocolPtr& in, void* out), (override));
};

class MockRpcServiceProxy : public RpcServiceProxy {
 public:
  stream::StreamReaderWriterProviderPtr SelectStreamProvider(const ClientContextPtr& context,
                                                             void* rpc_reply_msg = nullptr) override {
    if (!stream_) stream_ = testing::CreateMockStreamReaderWriterProvider();
    return stream_;
  }

  void SetMockServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) {
    SetServiceProxyOptionInner(option);
  }

  MOCK_METHOD(void, UnaryTransportInvoke, (const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp),
              (override));

  MOCK_METHOD(Future<ProtocolPtr>, AsyncUnaryTransportInvoke,
              (const ClientContextPtr& context, const ProtocolPtr& req_protocol), (override));

 protected:
  stream::StreamReaderWriterProviderPtr stream_;
};

using MockRpcServiceProxyPtr = std::shared_ptr<MockRpcServiceProxy>;

class RpcServiceProxyTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    codec_ = std::make_shared<MockTrpcClientCodec>();
    ClientCodecFactory::GetInstance()->Register(codec_);
    RegisterPlugins();

    detail::SetDefaultOption(option_);
    option_->name = "trpc.test.helloworld.Greeter";
    option_->caller_name = "Test.HelloWorldClient";
    option_->codec_name = "mock_codec";
    option_->conn_type = "long";
    option_->network = "tcp";
    option_->timeout = 1000;
    option_->target = "127.0.0.1:10001";
    option_->selector_name = "direct";
  }

  static void TearDownTestCase() { UnregisterPlugins(); }

 protected:
  void SetUp() override {
    mock_rpc_service_proxy_ = std::make_shared<MockRpcServiceProxy>();
    mock_rpc_service_proxy_->SetMockServiceProxyOption(option_);
  }

  void TearDown() override {
    mock_rpc_service_proxy_->Stop();
    mock_rpc_service_proxy_->Destroy();
  }

  ClientContextPtr GetClientContext() {
    auto ctx = MakeClientContext(mock_rpc_service_proxy_);
    ctx->SetAddr("127.0.0.1", 10001);
    return ctx;
  }

 protected:
  static std::shared_ptr<ServiceProxyOption> option_;
  static std::shared_ptr<MockTrpcClientCodec> codec_;
  MockRpcServiceProxyPtr mock_rpc_service_proxy_{nullptr};
};

std::shared_ptr<ServiceProxyOption> RpcServiceProxyTestFixture::option_ = std::make_shared<ServiceProxyOption>();
std::shared_ptr<MockTrpcClientCodec> RpcServiceProxyTestFixture::codec_ = nullptr;

// unary invoke ok
TEST_F(RpcServiceProxyTestFixture, UnaryInvokeOk) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(*codec_, FillResponse(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));

  ProtocolPtr rsp_data = codec_->CreateResponsePtr();
  EXPECT_CALL(*mock_rpc_service_proxy_, UnaryTransportInvoke(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::SetArgReferee<2>(rsp_data));

  auto client_context = GetClientContext();
  test::helloworld::HelloRequest hello_req;
  test::helloworld::HelloReply hello_rsp;
  Status status = mock_rpc_service_proxy_->UnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(
      client_context, hello_req, &hello_rsp);

  EXPECT_TRUE(status.OK());
}

// fillrequest failed
TEST_F(RpcServiceProxyTestFixture, UnaryInvokeFillRequestError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(false));

  auto client_context = GetClientContext();
  test::helloworld::HelloRequest hello_req;
  test::helloworld::HelloReply hello_rsp;
  Status status = mock_rpc_service_proxy_->UnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(
      client_context, hello_req, &hello_rsp);

  EXPECT_FALSE(status.OK());
  EXPECT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
  EXPECT_EQ(status.GetFuncRetCode(), 0);
}

// encode fail
TEST_F(RpcServiceProxyTestFixture, UnaryInvokeEncodeError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));

  auto client_context = GetClientContext();
  EXPECT_CALL(*mock_rpc_service_proxy_, UnaryTransportInvoke(::testing::_, ::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke([client_context](const ClientContextPtr&, const ProtocolPtr&, ProtocolPtr&) {
        client_context->SetStatus(Status(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, 0, "encode failed"));
      }));

  test::helloworld::HelloRequest hello_req;
  test::helloworld::HelloReply hello_rsp;
  Status ret_status =
      mock_rpc_service_proxy_->UnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(
          client_context, hello_req, &hello_rsp);

  EXPECT_FALSE(ret_status.OK());
  EXPECT_EQ(ret_status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
}

// invoke error
TEST_F(RpcServiceProxyTestFixture, UnaryInvokeInvokeError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));

  int func_ret = -3;
  auto client_context = GetClientContext();
  ProtocolPtr rsp_data = codec_->CreateResponsePtr();
  EXPECT_CALL(*mock_rpc_service_proxy_, UnaryTransportInvoke(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::DoAll(
          ::testing::SetArgReferee<2>(rsp_data),
          ::testing::Invoke([client_context, func_ret](const ClientContextPtr&, const ProtocolPtr&, ProtocolPtr&) {
            client_context->SetStatus(Status(TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, func_ret, "invoke failed"));
          })));

  test::helloworld::HelloRequest hello_req;
  test::helloworld::HelloReply hello_rsp;
  auto ret_status = mock_rpc_service_proxy_->UnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(
      client_context, hello_req, &hello_rsp);

  EXPECT_FALSE(ret_status.OK());
  EXPECT_EQ(ret_status.GetFuncRetCode(), func_ret);
}

// decode fail
TEST_F(RpcServiceProxyTestFixture, UnaryInvokeDecodeError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));

  auto client_context = GetClientContext();
  ProtocolPtr rsp_data = codec_->CreateResponsePtr();
  EXPECT_CALL(*mock_rpc_service_proxy_, UnaryTransportInvoke(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::DoAll(
          ::testing::SetArgReferee<2>(rsp_data),
          ::testing::Invoke([client_context](const ClientContextPtr&, const ProtocolPtr&, ProtocolPtr&) {
            client_context->SetStatus(Status(TrpcRetCode::TRPC_CLIENT_DECODE_ERR, 0, "decode failed"));
          })));

  test::helloworld::HelloRequest hello_req;
  test::helloworld::HelloReply hello_rsp;
  Status ret_status =
      mock_rpc_service_proxy_->UnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(
          client_context, hello_req, &hello_rsp);

  EXPECT_FALSE(ret_status.OK());
  EXPECT_EQ(ret_status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
}

// fill response faild
TEST_F(RpcServiceProxyTestFixture, UnaryInvokeFillResponseError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(*codec_, FillResponse(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(false));

  ProtocolPtr rsp_data = codec_->CreateResponsePtr();
  EXPECT_CALL(*mock_rpc_service_proxy_, UnaryTransportInvoke(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::SetArgReferee<2>(rsp_data));

  auto client_context = GetClientContext();
  test::helloworld::HelloRequest hello_req;
  test::helloworld::HelloReply hello_rsp;
  Status status = mock_rpc_service_proxy_->UnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(
      client_context, hello_req, &hello_rsp);

  EXPECT_FALSE(status.OK());
  EXPECT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
}

// PbSerializedReqUnaryInvoke
TEST_F(RpcServiceProxyTestFixture, PbSerializedReqUnaryInvokeNormal) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::AtLeast(1))
      .WillOnce(::testing::Return(true));

  EXPECT_CALL(*codec_, FillResponse(::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::AtLeast(1))
      .WillOnce(::testing::Return(true));

  ProtocolPtr rsp_data = codec_->CreateResponsePtr();
  EXPECT_CALL(*mock_rpc_service_proxy_, UnaryTransportInvoke(::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::AtLeast(1))
      .WillOnce(::testing::SetArgReferee<2>(rsp_data));

  // Serialize the PB request into a non-contiguous buffer.
  test::helloworld::HelloRequest hello_req;
  NoncontiguousBuffer pb_serialized_buffer;
  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream nbos(&builder);
  hello_req.SerializePartialToZeroCopyStream(&nbos);
  nbos.Flush();
  pb_serialized_buffer = builder.DestructiveGet();

  auto client_context = GetClientContext();
  test::helloworld::HelloReply hello_rsp;
  Status status =
      mock_rpc_service_proxy_->PbSerializedReqUnaryInvoke(client_context, std::move(pb_serialized_buffer), &hello_rsp);
  EXPECT_EQ(status.GetFrameworkRetCode(), 0);
}

// async unary invoke ok
TEST_F(RpcServiceProxyTestFixture, AsyncUnaryInvokeOk) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(*codec_, FillResponse(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mock_rpc_service_proxy_, AsyncUnaryTransportInvoke(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<ProtocolPtr>(codec_->CreateResponsePtr()))));

  auto client_context = GetClientContext();
  test::helloworld::HelloRequest hello_req;
  auto fut =
      mock_rpc_service_proxy_
          ->AsyncUnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(client_context, hello_req)
          .Then([client_context](Future<test::helloworld::HelloReply>&& fut) {
            EXPECT_TRUE(fut.IsReady());
            Status status = client_context->GetStatus();
            EXPECT_TRUE(status.OK());
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(fut));
}

TEST_F(RpcServiceProxyTestFixture, AsyncUnaryInvokeError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));

  // response error
  EXPECT_CALL(*mock_rpc_service_proxy_, AsyncUnaryTransportInvoke(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(::testing::ByMove(MakeExceptionFuture<ProtocolPtr>(
          CommonException("invoke error", TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR)))));

  auto client_context = GetClientContext();
  test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("test");

  auto fut =
      mock_rpc_service_proxy_
          ->AsyncUnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(client_context, hello_req)
          .Then([](Future<test::helloworld::HelloReply>&& fut) {
            EXPECT_TRUE(fut.IsFailed());
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(fut));
}

// async unary invoke when fill request error
TEST_F(RpcServiceProxyTestFixture, AsyncUnaryInvokeFillRequestError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(false));

  auto client_context = GetClientContext();
  test::helloworld::HelloRequest hello_req;
  auto fut =
      mock_rpc_service_proxy_
          ->AsyncUnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(client_context, hello_req)
          .Then([client_context](Future<test::helloworld::HelloReply>&& fut) {
            EXPECT_TRUE(fut.IsFailed());
            Status status = client_context->GetStatus();
            EXPECT_FALSE(status.OK());
            EXPECT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(fut));
}

// async unary invoke when fill response error
TEST_F(RpcServiceProxyTestFixture, AsyncUnaryInvokeFillResponseError) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(true));
  EXPECT_CALL(*codec_, FillResponse(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(false));
  EXPECT_CALL(*mock_rpc_service_proxy_, AsyncUnaryTransportInvoke(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<ProtocolPtr>(codec_->CreateResponsePtr()))));

  auto client_context = GetClientContext();
  test::helloworld::HelloRequest hello_req;
  auto fut =
      mock_rpc_service_proxy_
          ->AsyncUnaryInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(client_context, hello_req)
          .Then([client_context](Future<test::helloworld::HelloReply>&& fut) {
            EXPECT_TRUE(fut.IsFailed());
            Status status = client_context->GetStatus();
            EXPECT_FALSE(status.OK());
            EXPECT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(fut));
}

TEST_F(RpcServiceProxyTestFixture, AsyncStreamInvoke1) {
  auto client_context = GetClientContext();
  auto stream = dynamic_pointer_cast<testing::MockStreamReaderWriterProvider>(
      mock_rpc_service_proxy_->SelectStreamProvider(client_context));
  EXPECT_CALL(*stream, AsyncStart()).WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));
  EXPECT_CALL(*stream, AsyncRead(1000))
      .WillOnce(::testing::Return(::testing::ByMove(
          MakeReadyFuture<std::optional<NoncontiguousBuffer>>(std::make_optional<NoncontiguousBuffer>()))));

  client_context->SetTimeout(1000);
  auto fut = mock_rpc_service_proxy_
                 ->AsyncStreamInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(client_context)
                 .Then([](std::pair<stream::AsyncWriterPtr<test::helloworld::HelloRequest>,
                                    Future<test::helloworld::HelloReply>>&& p) {
                   EXPECT_TRUE(p.second.IsReady());
                   return MakeReadyFuture<>();
                 });
  fut = future::BlockingGet(std::move(fut));
  EXPECT_TRUE(fut.IsReady());
}

TEST_F(RpcServiceProxyTestFixture, AsyncStreamInvoke2) {
  auto client_context = GetClientContext();
  auto stream = dynamic_pointer_cast<testing::MockStreamReaderWriterProvider>(
      mock_rpc_service_proxy_->SelectStreamProvider(client_context));
  EXPECT_CALL(*stream, AsyncStart()).WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));
  EXPECT_CALL(*stream, AsyncWrite(::testing::_)).WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));

  auto fut = mock_rpc_service_proxy_->AsyncStreamInvoke<test::helloworld::HelloRequest, test::helloworld::HelloReply>(
      client_context, test::helloworld::HelloRequest{});
  fut = future::BlockingGet(std::move(fut));
  EXPECT_TRUE(fut.IsReady());
}

TEST_F(RpcServiceProxyTestFixture, SetMessageEncodeType) {
  EXPECT_CALL(*codec_, FillRequest(::testing::_, ::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(false));

  // Transparent
  {
    auto context = MakeClientContext(mock_rpc_service_proxy_);
    context->SetTransparent(true);
    NoncontiguousBuffer req, rsp;
    mock_rpc_service_proxy_->UnaryInvoke(context, req, &rsp);
    EXPECT_EQ(context->GetReqEncodeType(), 0);
    EXPECT_EQ(context->GetReqEncodeDataType(), 0);
  }

  // pb
  {
    auto context = MakeClientContext(mock_rpc_service_proxy_);
    test::helloworld::HelloRequest req;
    test::helloworld::HelloReply rsp;
    mock_rpc_service_proxy_->UnaryInvoke(context, req, &rsp);
    EXPECT_EQ(context->GetReqEncodeType(), serialization::kPbType);
    EXPECT_EQ(context->GetReqEncodeDataType(), serialization::kPbMessage);
  }

  // flat buffer
  {
    auto context = MakeClientContext(mock_rpc_service_proxy_);
    flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest> req;
    flatbuffers::trpc::Message<trpc::test::helloworld::FbReply> rsp;
    mock_rpc_service_proxy_->UnaryInvoke(context, req, &rsp);
    EXPECT_EQ(context->GetReqEncodeType(), serialization::kFlatBuffersType);
    EXPECT_EQ(context->GetReqEncodeDataType(), serialization::kFlatBuffers);
  }

  // json
  {
    auto context = MakeClientContext(mock_rpc_service_proxy_);
    rapidjson::Document req, rsp;
    mock_rpc_service_proxy_->UnaryInvoke(context, req, &rsp);
    EXPECT_EQ(context->GetReqEncodeType(), serialization::kJsonType);
    EXPECT_EQ(context->GetReqEncodeDataType(), serialization::kRapidJson);
  }

  // string
  {
    auto context = MakeClientContext(mock_rpc_service_proxy_);
    std::string req, rsp;
    mock_rpc_service_proxy_->UnaryInvoke(context, req, &rsp);
    EXPECT_EQ(context->GetReqEncodeType(), serialization::kNoopType);
    EXPECT_EQ(context->GetReqEncodeDataType(), serialization::kStringNoop);
  }

  // NonContiguousBuffer
  {
    auto context = MakeClientContext(mock_rpc_service_proxy_);
    NoncontiguousBuffer req;
    NoncontiguousBuffer rsp;
    mock_rpc_service_proxy_->UnaryInvoke(context, req, &rsp);
    EXPECT_EQ(context->GetReqEncodeType(), serialization::kNoopType);
    EXPECT_EQ(context->GetReqEncodeDataType(), serialization::kNonContiguousBufferNoop);
  }

  // no supported type
  {
    auto context = MakeClientContext(mock_rpc_service_proxy_);
    int req, rsp;
    mock_rpc_service_proxy_->UnaryInvoke(context, req, &rsp);
    EXPECT_FALSE(context->GetStatus().OK());
    EXPECT_EQ(context->GetStatus().GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
  }
}

}  // namespace trpc::testing
