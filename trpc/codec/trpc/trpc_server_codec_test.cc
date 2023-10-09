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

#include "trpc/codec/trpc/trpc_server_codec.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/server/server_context.h"

namespace trpc::testing {

class TrpcServerCodecTest : public ::testing::Test {
 public:
  TrpcServerCodec codec_;
};

TEST_F(TrpcServerCodecTest, TrpcServerCodecName) { ASSERT_EQ("trpc", codec_.Name()); }

TEST_F(TrpcServerCodecTest, TrpcServerCodecEncode) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();

  ProtocolPtr req = codec_.CreateRequestObject();
  ProtocolPtr rsp = codec_.CreateResponseObject();

  auto* trpc_req = dynamic_cast<TrpcRequestProtocol*>(req.get());

  FillTrpcRequestProtocolData(*trpc_req);

  context->SetRequestMsg(std::move(req));

  ASSERT_EQ(context->GetRequestMsg().get(), trpc_req);

  NoncontiguousBuffer buff;

  ASSERT_TRUE(codec_.ZeroCopyEncode(context, rsp, buff));
}

TEST_F(TrpcServerCodecTest, TrpcServerCodecDecode) {
  TrpcRequestProtocol req;
  auto req_atta = trpc::CreateBufferSlow("test");
  size_t req_atta_size = req_atta.ByteSize();
  req.SetProtocolAttachment(std::move(req_atta));

  FillTrpcRequestProtocolData(req);
  NoncontiguousBuffer buff;

  ASSERT_EQ(true, req.ZeroCopyEncode(buff));

  ServerContextPtr context = MakeRefCounted<ServerContext>();

  ProtocolPtr trpc_req = codec_.CreateRequestObject();

  context->SetRequestMsg(std::move(trpc_req));

  context->SetResponseMsg(codec_.CreateResponseObject());

  ASSERT_TRUE(codec_.ZeroCopyDecode(context, std::move(buff), context->GetRequestMsg()));
  ASSERT_EQ(req_atta_size, context->GetRequestAttachment().ByteSize());
}

TEST_F(TrpcServerCodecTest, TrpcServerCodecDecode_Fail) {
  TrpcRequestProtocol req;
  auto req_atta = trpc::CreateBufferSlow("test");
  req.SetProtocolAttachment(std::move(req_atta));

  FillTrpcRequestProtocolData(req);
  NoncontiguousBuffer buff;

  ASSERT_EQ(true, req.ZeroCopyEncode(buff));
  NoncontiguousBuffer partial_buff = buff.Cut(buff.ByteSize() - 1);

  ServerContextPtr context = MakeRefCounted<ServerContext>();

  ProtocolPtr trpc_req = codec_.CreateRequestObject();

  context->SetRequestMsg(std::move(trpc_req));

  context->SetResponseMsg(codec_.CreateResponseObject());

  ASSERT_FALSE(codec_.ZeroCopyDecode(context, std::move(partial_buff), context->GetRequestMsg()));
  ASSERT_EQ(0, context->GetRequestAttachment().ByteSize());
}

TEST_F(TrpcServerCodecTest, TrpcServerCodecRetcode) {
  // Timeout case.
  ServerContextPtr context = MakeRefCounted<ServerContext>();

  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());
  ProtocolPtr& req = context->GetRequestMsg();
  req->SetRequestId(1);
  ProtocolPtr& rsp = context->GetResponseMsg();
  context->GetStatus().SetFrameworkRetCode(codec_.GetProtocolRetCode(trpc::codec::ServerRetCode::TIMEOUT_ERROR));

  NoncontiguousBuffer send_data;
  bool ret = codec_.ZeroCopyEncode(context, rsp, send_data);
  ASSERT_TRUE(ret);

  auto* trpc_rsp = dynamic_cast<TrpcResponseProtocol*>(rsp.get());
  int framwork_ret = trpc_rsp->rsp_header.ret();
  ASSERT_EQ(framwork_ret, TrpcRetCode::TRPC_SERVER_TIMEOUT_ERR);
}

TEST_F(TrpcServerCodecTest, PickProtocolMessageMetadataWithTrue) {
  TrpcFixedHeader fixed_header;
  fixed_header.data_frame_type = 1;
  fixed_header.stream_frame_type = 1;
  fixed_header.stream_id = 100;

  NoncontiguousBufferBuilder builder;
  auto header_buffer = builder.Reserve(fixed_header.ByteSizeLong());
  ASSERT_TRUE(fixed_header.Encode(header_buffer));

  std::any message = builder.DestructiveGet();
  std::any  meta = TrpcProtocolMessageMetadata{};
  ASSERT_TRUE(codec_.Pick(message, meta));

  auto& metadata = std::any_cast<TrpcProtocolMessageMetadata&>(meta);
  ASSERT_EQ(fixed_header.data_frame_type, metadata.data_frame_type);
  ASSERT_EQ(fixed_header.stream_frame_type, metadata.stream_frame_type);
  ASSERT_EQ(fixed_header.stream_id, metadata.stream_id);
  ASSERT_TRUE(metadata.enable_stream);
}

TEST_F(TrpcServerCodecTest, HasStreamingRpcWithTrue) { ASSERT_TRUE(codec_.HasStreamingRpc()); }

TEST_F(TrpcServerCodecTest, IsStreamingProtocolWithFalse) { ASSERT_FALSE(codec_.IsStreamingProtocol()); }

}  // namespace trpc::testing
