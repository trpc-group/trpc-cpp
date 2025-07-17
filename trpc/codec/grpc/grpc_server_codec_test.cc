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

#include "trpc/codec/grpc/grpc_server_codec.h"

#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/grpc_stream_frame.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/response.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/server/server_context.h"
#include "trpc/stream/testing/mock_connection.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/util/buffer/zero_copy_stream.h"

namespace trpc::testing {

namespace {
using HelloRequest = trpc::test::helloworld::HelloRequest;
using HelloReply = trpc::test::helloworld::HelloReply;

template <typename Message>
void SerializeProtobufMessage(const Message& msg, NoncontiguousBuffer* buffer) {
  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream nbos(&builder);

  msg.SerializePartialToZeroCopyStream(&nbos);
  nbos.Flush();

  *buffer = builder.DestructiveGet();
}
}  // namespace

class GrpcServerCodecTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    compressor::Init();

    conn_ = MakeRefCounted<Connection>();
    conn_->SetConnType(ConnectionType::kTcpLong);
  }

  static void TearDownTestCase() { trpc::compressor::Destroy(); }

 protected:
  void SetUp() override {
    codec_ = std::make_unique<GrpcServerCodec>();
    context_ = MakeRefCounted<ServerContext>();
    context_->SetRequestMsg(codec_->CreateRequestObject());
    context_->SetResponseMsg(codec_->CreateResponseObject());
  }

  void TearDown() override {}

 protected:
  std::unique_ptr<GrpcServerCodec> codec_{nullptr};
  ServerContextPtr context_;
  static ConnectionPtr conn_;
};

ConnectionPtr GrpcServerCodecTest::conn_ = nullptr;

TEST_F(GrpcServerCodecTest, Name) { ASSERT_EQ(codec_->Name(), "grpc"); }


TEST_F(GrpcServerCodecTest, HasStreamingRpc) { ASSERT_TRUE(codec_->HasStreamingRpc()); }

TEST_F(GrpcServerCodecTest, IsStreamingProtocol) { ASSERT_TRUE(codec_->IsStreamingProtocol()); }

TEST_F(GrpcServerCodecTest, Pick) {
  stream::GrpcRequestPacket unary_packet;
  std::any meta = GrpcProtocolMessageMetadata{};
  ASSERT_TRUE(codec_->Pick(unary_packet, meta));

  auto& metadata = std::any_cast<GrpcProtocolMessageMetadata&>(meta);
  ASSERT_FALSE(metadata.enable_stream);

  stream::GrpcRequestPacket stream_packet;
  stream_packet.frame = MakeRefCounted<stream::GrpcStreamCloseFrame>(1);
  ASSERT_TRUE(codec_->Pick(stream_packet, meta));

  metadata = std::any_cast<GrpcProtocolMessageMetadata&>(meta);
  ASSERT_TRUE(metadata.enable_stream);
  ASSERT_EQ(static_cast<uint8_t>(stream_packet.frame->GetFrameType()), metadata.stream_frame_type);
  ASSERT_EQ(stream_packet.frame->GetStreamId(), metadata.stream_id);
}

TEST_F(GrpcServerCodecTest, ZeroCopyCheckNotOk) {
  NoncontiguousBuffer in{};
  std::deque<std::any> out{};

  ASSERT_EQ(codec_->ZeroCopyCheck(conn_, in, out), PacketChecker::PACKET_LESS);
  ASSERT_TRUE(out.empty());
}

TEST_F(GrpcServerCodecTest, ZeroCopyDecodeOk) {
  HelloRequest hello_request{};
  hello_request.set_msg("hello world");
  NoncontiguousBuffer request_content;
  SerializeProtobufMessage(hello_request, &request_content);
  compressor::CompressIfNeeded(compressor::kGzip, request_content);

  GrpcMessageContent grpc_msg_content;
  grpc_msg_content.compressed = 1;
  grpc_msg_content.length = request_content.ByteSize();
  grpc_msg_content.content = std::move(request_content);
  ASSERT_TRUE(grpc_msg_content.Encode(&request_content));

  stream::GrpcRequestPacket packet;
  packet.req = http2::CreateRequest();
  http2::RequestPtr& http2_request = packet.req;
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
  http2_request->SetNonContiguousBufferContent(std::move(request_content));

  auto& grpc_request = context_->GetRequestMsg();
  ASSERT_TRUE(codec_->ZeroCopyDecode(context_, packet, grpc_request));

  const auto& kv_pairs = grpc_request->GetKVInfos();
  ASSERT_EQ(kv_pairs.size(), 2);
  auto user_defined = kv_pairs.find("x-user-defined-01");
  ASSERT_TRUE(user_defined != kv_pairs.end());
  ASSERT_EQ(user_defined->second, "Tom");
}

TEST_F(GrpcServerCodecTest, ZeroCopyDecodeFailedDueToEmptyMessageContent) {
  stream::GrpcRequestPacket packet;
  packet.req = http2::CreateRequest();
  auto& grpc_request = context_->GetRequestMsg();
  ASSERT_FALSE(codec_->ZeroCopyDecode(context_, packet, grpc_request));
}

TEST_F(GrpcServerCodecTest, ZeroCopyEncodeOk) {
  HelloReply hello_reply{};
  hello_reply.set_msg("hello world");
  NoncontiguousBuffer response_content{};
  SerializeProtobufMessage(hello_reply, &response_content);

  context_->SetStatus(Status{0, -1, "internal error"});

  auto grpc_response = context_->GetResponseMsg();
  grpc_response->SetKVInfo("x-user-defined-01", "Tom");
  grpc_response->SetKVInfo("x-user-defined-02", "Jerry");
  grpc_response->SetNonContiguousProtocolBody(std::move(response_content));
  ASSERT_TRUE(codec_->ZeroCopyEncode(context_, grpc_response, response_content));

  const auto& http2_response = static_cast<GrpcUnaryResponseProtocol*>(grpc_response.get())->GetHttp2Response();
  int status_code{0};
  ASSERT_TRUE(internal::GetHeaderInteger(http2_response->GetTrailer(), kGrpcStatusName, &status_code));
  ASSERT_EQ(status_code, -1);

  std::string header_value{""};
  ASSERT_TRUE(internal::GetHeaderString(http2_response->GetTrailer(), "x-user-defined-01", &header_value));
  ASSERT_EQ(header_value, "Tom");
  ASSERT_TRUE(internal::GetHeaderString(http2_response->GetTrailer(),"x-user-defined-02", &header_value));
  ASSERT_EQ(header_value, "Jerry");
}

TEST_F(GrpcServerCodecTest, CreateRequestObject) { ASSERT_NE(codec_->CreateRequestObject(), nullptr); }

TEST_F(GrpcServerCodecTest, CreateResponseObject) { ASSERT_NE(codec_->CreateResponseObject(), nullptr); }

}  // namespace trpc::testing
