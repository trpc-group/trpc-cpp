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

#include "trpc/codec/grpc/grpc_client_codec.h"

#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/response.h"
#include "trpc/compressor/compressor.h"
#include "trpc/compressor/compressor_factory.h"
#include "trpc/compressor/testing/compressor_testing.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/util/buffer/zero_copy_stream.h"

namespace trpc::testing {

namespace {
using HelloRequest = trpc::test::helloworld::HelloRequest;
using HelloReply = trpc::test::helloworld::HelloReply;
using trpc::compressor::testing::MockCompressor;

template <typename Message>
void SerializeProtobufMessage(const Message& msg, NoncontiguousBuffer* buffer) {
  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream nbos(&builder);

  msg.SerializePartialToZeroCopyStream(&nbos);
  nbos.Flush();

  *buffer = std::move(builder.DestructiveGet());
}

}  // namespace

class GrpcClientCodecTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::serialization::Init();

    conn_ = MakeRefCounted<MockTcpConnection>();
    conn_->SetConnType(ConnectionType::kTcpLong);
  }

  static void TearDownTestCase() { trpc::serialization::Destroy(); }

 protected:
  void SetUp() override {
    codec_ = std::make_shared<GrpcClientCodec>();
    client_context_ = MakeRefCounted<ClientContext>();
  }

  void TearDown() override {}

 protected:
  std::shared_ptr<GrpcClientCodec> codec_{nullptr};
  ClientContextPtr client_context_;
  static ConnectionPtr conn_;
};

ConnectionPtr GrpcClientCodecTest::conn_ = nullptr;

TEST_F(GrpcClientCodecTest, Name) { ASSERT_EQ("grpc", codec_->Name()); }

TEST_F(GrpcClientCodecTest, HasStreamingRpc) { ASSERT_FALSE(codec_->HasStreamingRpc()); }

TEST_F(GrpcClientCodecTest, IsStreamingProtocol) { ASSERT_TRUE(codec_->IsStreamingProtocol()); }

TEST_F(GrpcClientCodecTest, Pick) {
  std::any a;
  ASSERT_FALSE(codec_->Pick(nullptr, a));
}

TEST_F(GrpcClientCodecTest, ZeroCopyCheckNotOk) {
  NoncontiguousBuffer in{};
  std::deque<std::any> out{};

  ASSERT_EQ(codec_->ZeroCopyCheck(conn_, in, out), PacketChecker::PACKET_LESS);
  ASSERT_TRUE(out.empty());
}

TEST_F(GrpcClientCodecTest, ZeroCopyEncodeOk) {
  auto context = MakeRefCounted<ClientContext>(codec_);
  context->SetReqEncodeType(serialization::kPbMessage);
  context->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");

  HelloRequest hello_request;
  hello_request.set_msg("hello world");

  auto grpc_request = codec_->CreateRequestPtr();
  grpc_request->SetKVInfo("x-user-defined-01", "Tom");
  grpc_request->SetKVInfo("x-user-defined-02", "Jerry");
  NoncontiguousBuffer buffer{};

  ASSERT_TRUE(codec_->FillRequest(context, grpc_request, &hello_request));
  ASSERT_TRUE(codec_->ZeroCopyEncode(context, grpc_request, buffer));
}

TEST_F(GrpcClientCodecTest, ZeroCopyDecodeOk) {
  auto context = MakeRefCounted<ClientContext>(codec_);
  context->SetReqEncodeType(serialization::kPbMessage);

  HelloReply hello_reply{};
  hello_reply.set_msg("hello world");
  NoncontiguousBuffer buffer{};
  SerializeProtobufMessage(hello_reply, &buffer);

  GrpcMessageContent grpc_msg_content;
  grpc_msg_content.compressed = 0;
  grpc_msg_content.length = buffer.ByteSize();
  grpc_msg_content.content = std::move(buffer);

  ASSERT_TRUE(grpc_msg_content.Encode(&buffer));

  auto http2_response = http2::CreateResponse();
  http2_response->SetStatus(200);
  http2_response->GetMutableTrailer()->Add("grpc-status", "0");
  http2_response->GetMutableTrailer()->Add("grpc-func-ret", "0");
  http2_response->GetMutableTrailer()->Add("grpc-message", "OK");
  http2_response->GetMutableTrailer()->Add("x-user-defined-01", "Tom");
  http2_response->GetMutableTrailer()->Add("x-user-defined-02", "Jerry");
  http2_response->SetNonContiguousBufferContent(std::move(buffer));

  auto grpc_response = codec_->CreateResponsePtr();
  ASSERT_TRUE(codec_->ZeroCopyDecode(context, std::move(http2_response), grpc_response));

  const auto& kv_pairs = grpc_response->GetKVInfos();
  ASSERT_EQ(kv_pairs.size(), 2);
  auto user_defined = kv_pairs.find("x-user-defined-01");
  ASSERT_TRUE(user_defined != kv_pairs.end());
  ASSERT_EQ(user_defined->second, "Tom");
}

TEST_F(GrpcClientCodecTest, ZeroCopyDecodeFailedDueToBadStatusCode) {
  auto context = MakeRefCounted<ClientContext>();
  auto http2_response = http2::CreateResponse();
  http2_response->SetStatus(404);
  http2_response->AddHeader("grpc-status", "0");
  http2_response->AddHeader("grpc-func-ret", "0");
  http2_response->AddHeader("grpc-message", "not found");

  auto grpc_response = codec_->CreateResponsePtr();
  ASSERT_FALSE(codec_->ZeroCopyDecode(context, std::move(http2_response), grpc_response));
}

TEST_F(GrpcClientCodecTest, FillRequestOk) {
  compressor::Init();

  auto context = MakeRefCounted<ClientContext>();
  auto request = codec_->CreateRequestPtr();
  context->SetRequest(request);
  context->SetReqEncodeType(serialization::kPbMessage);
  context->SetReqCompressType(compressor::kGzip);

  HelloRequest hello_request;
  hello_request.set_msg("hello world");

  ASSERT_TRUE(codec_->FillRequest(context, request, &hello_request));

  compressor::Destroy();
}

TEST_F(GrpcClientCodecTest, FillResponseOk) {
  compressor::Init();

  HelloReply hello_reply{};
  hello_reply.set_msg("hello world");
  NoncontiguousBuffer buffer{};
  SerializeProtobufMessage(hello_reply, &buffer);
  compressor::CompressIfNeeded(compressor::kGzip, buffer);

  GrpcMessageContent grpc_msg_content;
  grpc_msg_content.compressed = 1;
  grpc_msg_content.length = buffer.ByteSize();
  grpc_msg_content.content = std::move(buffer);

  ASSERT_TRUE(grpc_msg_content.Encode(&buffer));
  auto grpc_unary_response = codec_->CreateResponsePtr();
  grpc_unary_response->SetNonContiguousProtocolBody(std::move(buffer));
  auto context = MakeRefCounted<ClientContext>(codec_);
  context->SetRspCompressType(compressor::kGzip);
  context->SetRspEncodeType(serialization::kPbType);
  context->SetRspEncodeDataType(serialization::kPbMessage);

  context->SetResponse(grpc_unary_response);

  HelloReply got_hello_reply{};
  ASSERT_TRUE(codec_->FillResponse(context, grpc_unary_response, &got_hello_reply));
  ASSERT_EQ(hello_reply.msg(), got_hello_reply.msg());

  compressor::Destroy();
}

TEST_F(GrpcClientCodecTest, SerializeFailedDueToUnsupportedSerializationType) {
  auto context = MakeRefCounted<ClientContext>();
  uint8_t unsupported_serialization_type = serialization::kMaxType;
  context->SetReqEncodeType(unsupported_serialization_type);

  auto grpc_request = codec_->CreateRequestPtr();

  HelloRequest hello_request;
  ASSERT_FALSE(codec_->FillRequest(context, grpc_request, &hello_request));

  auto& status = context->GetStatus();
  ASSERT_FALSE(status.OK());
  ASSERT_EQ(static_cast<int>(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR), status.GetFrameworkRetCode());
}

TEST_F(GrpcClientCodecTest, SerializeOk) {
  auto context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kPbMessage);

  auto request = codec_->CreateRequestPtr();
  context->SetRequest(request);

  HelloRequest hello_request;
  hello_request.set_msg("hello world");

  NoncontiguousBuffer buffer{};
  ASSERT_TRUE(codec_->FillRequest(context, request, &hello_request));

  ASSERT_GT(request->GetMessageSize(), 0);
}

TEST_F(GrpcClientCodecTest, DeserializeFailedDueToUnsupportedSerializationType) {
  auto context = MakeRefCounted<ClientContext>();
  uint8_t unsupported_serialization_type = serialization::kMaxType;
  context->SetReqEncodeType(unsupported_serialization_type);

  auto grpc_unary_response = codec_->CreateResponsePtr();
  HelloReply got_hello_reply{};
  ASSERT_FALSE(codec_->FillResponse(context, grpc_unary_response, &got_hello_reply));

  auto& status = context->GetStatus();
  ASSERT_FALSE(status.OK());
  ASSERT_EQ(static_cast<int>(TrpcRetCode::TRPC_CLIENT_DECODE_ERR), status.GetFrameworkRetCode());
}

TEST_F(GrpcClientCodecTest, DeserializeFailedDueToBadBufferContent) {
  auto context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kPbMessage);

  HelloReply hello_reply;
  NoncontiguousBufferBuilder buffer_builder{};
  buffer_builder.Append("hello world");
  auto buffer = buffer_builder.DestructiveGet();

  auto grpc_unary_response = codec_->CreateResponsePtr();
  grpc_unary_response->SetNonContiguousProtocolBody(std::move(buffer));
  HelloReply got_hello_reply{};
  ASSERT_FALSE(codec_->FillResponse(context, grpc_unary_response, &got_hello_reply));

  auto& status = context->GetStatus();
  ASSERT_FALSE(status.OK());
  ASSERT_EQ(static_cast<int>(TrpcRetCode::TRPC_CLIENT_DECODE_ERR), status.GetFrameworkRetCode());
}

TEST_F(GrpcClientCodecTest, DeserializeOk) {
  auto context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kPbMessage);

  HelloReply hello_reply{};
  hello_reply.set_msg("hello world");
  NoncontiguousBuffer buffer{};
  SerializeProtobufMessage(hello_reply, &buffer);

  GrpcMessageContent grpc_msg_content;
  grpc_msg_content.compressed = 1;
  grpc_msg_content.length = buffer.ByteSize();
  grpc_msg_content.content = std::move(buffer);

  ASSERT_TRUE(grpc_msg_content.Encode(&buffer));
  auto grpc_unary_response = codec_->CreateResponsePtr();
  grpc_unary_response->SetNonContiguousProtocolBody(std::move(buffer));
  context->SetResponse(grpc_unary_response);

  HelloReply got_hello_reply;
  ASSERT_TRUE(codec_->FillResponse(context, grpc_unary_response, &got_hello_reply));
  ASSERT_EQ(got_hello_reply.msg(), hello_reply.msg());
}

TEST_F(GrpcClientCodecTest, CompressFailed) {
  auto compressor = MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*compressor, Type()).WillOnce(::testing::Return(compressor::kGzip));
  compressor::CompressorFactory::GetInstance()->Register(compressor);
  EXPECT_CALL(*compressor, DoCompress(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(false));

  auto context = MakeRefCounted<ClientContext>();
  auto request = codec_->CreateRequestPtr();
  context->SetRequest(request);
  context->SetReqCompressType(compressor::kGzip);

  HelloRequest hello_request;
  hello_request.set_msg("hello world");
  NoncontiguousBuffer buffer;
  SerializeProtobufMessage(hello_request, &buffer);

  ASSERT_FALSE(codec_->FillRequest(context, request, &hello_request));
  auto& status = context->GetStatus();
  ASSERT_FALSE(status.OK());
  ASSERT_EQ(static_cast<int>(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR), status.GetFrameworkRetCode());

  compressor::CompressorFactory::GetInstance()->Clear();
}

TEST_F(GrpcClientCodecTest, CompressOk) {
  compressor::Init();

  auto context = MakeRefCounted<ClientContext>();
  auto request = codec_->CreateRequestPtr();
  context->SetRequest(request);
  context->SetReqCompressType(compressor::kGzip);

  HelloRequest hello_request;
  hello_request.set_msg("hello world");

  ASSERT_TRUE(codec_->FillRequest(context, request, &hello_request));

  compressor::Destroy();
}

TEST_F(GrpcClientCodecTest, DecompressFailed) {
  auto compressor = MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*compressor, Type()).WillOnce(::testing::Return(compressor::kGzip));
  compressor::CompressorFactory::GetInstance()->Register(compressor);
  EXPECT_CALL(*compressor, DoDecompress(::testing::_, ::testing::_)).WillOnce(::testing::Return(false));

  auto context = MakeRefCounted<ClientContext>();
  uint8_t serialization_type = serialization::kPbMessage;
  context->SetReqEncodeType(serialization_type);
  context->SetRequest(codec_->CreateRequestPtr());

  HelloReply hello_reply{};
  hello_reply.set_msg("hello world");
  NoncontiguousBuffer buffer{};
  SerializeProtobufMessage(hello_reply, &buffer);
  GrpcMessageContent grpc_msg_content;
  grpc_msg_content.compressed = 1;
  grpc_msg_content.length = buffer.ByteSize();
  grpc_msg_content.content = std::move(buffer);
  ASSERT_TRUE(grpc_msg_content.Encode(&buffer));

  uint8_t compress_type = compressor::kGzip;
  auto grpc_unary_response = codec_->CreateResponsePtr();
  grpc_unary_response->SetNonContiguousProtocolBody(std::move(buffer));
  context->SetReqCompressType(compress_type);
  context->SetResponse(grpc_unary_response);
  context->SetRspCompressType(compress_type);

  HelloReply got_hello_reply{};
  ASSERT_FALSE(codec_->FillResponse(context, grpc_unary_response, &got_hello_reply));

  auto& status = context->GetStatus();
  ASSERT_FALSE(status.OK());
  ASSERT_EQ(static_cast<int>(TrpcRetCode::TRPC_CLIENT_DECODE_ERR), status.GetFrameworkRetCode());

  compressor::CompressorFactory::GetInstance()->Clear();
}

TEST_F(GrpcClientCodecTest, DecompressOk) {
  compressor::Init();

  auto context = MakeRefCounted<ClientContext>();
  auto request = codec_->CreateRequestPtr();
  context->SetRequest(request);
  context->SetReqCompressType(compressor::kGzip);

  HelloRequest hello_request;
  hello_request.set_msg("hello world");

  ASSERT_TRUE(codec_->FillRequest(context, request, &hello_request));

  uint8_t compress_type = compressor::kGzip;
  auto grpc_unary_response = codec_->CreateResponsePtr();
  grpc_unary_response->SetNonContiguousProtocolBody(request->GetNonContiguousProtocolBody());
  context->SetResponse(grpc_unary_response);
  context->SetRspCompressType(compress_type);

  HelloReply got_hello_reply{};
  ASSERT_TRUE(codec_->FillResponse(context, grpc_unary_response, &got_hello_reply));

  ASSERT_EQ(got_hello_reply.msg(), hello_request.msg());

  compressor::Destroy();
}

}  // namespace trpc::testing
