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

#include "trpc/codec/redis/redis_client_codec.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/redis/reply.h"
#include "trpc/client/redis/request.h"
#include "trpc/codec/redis/redis_protocol.h"

namespace trpc {

namespace testing {

class RedisClientCodecTest : public ::testing::Test {
 public:
  RedisClientCodec codec_;
};

TEST_F(RedisClientCodecTest, Name) {
  EXPECT_EQ("redis", codec_.Name());
  EXPECT_TRUE(codec_.SupportZeroCopy());
  EXPECT_EQ(true, codec_.SupportPipeline());
}

TEST_F(RedisClientCodecTest, Decode) {
  redis::Reply r = redis::Reply();
  ClientContextPtr ctx = MakeRefCounted<ClientContext>();
  auto out = codec_.CreateResponsePtr();
  EXPECT_TRUE(codec_.ZeroCopyDecode(ctx, std::move(r), out));
}

TEST_F(RedisClientCodecTest, DecodeFail) {
  int r = 0;
  ClientContextPtr ctx = MakeRefCounted<ClientContext>();
  auto out = codec_.CreateResponsePtr();
  EXPECT_FALSE(codec_.ZeroCopyDecode(ctx, r, out));
}

TEST_F(RedisClientCodecTest, DecodeWithContext) {
  ClientContextPtr ctx = MakeRefCounted<ClientContext>();
  trpc::redis::Reply rsp = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");

  std::any in = rsp;
  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();

  EXPECT_TRUE(codec_.ZeroCopyDecode(ctx, std::move(in), rsp_protocol));
  redis::Reply reply;
  EXPECT_TRUE(codec_.FillResponse(ctx, rsp_protocol, reinterpret_cast<void*>(&reply)));
  EXPECT_EQ(std::string("redis"), reply.GetString());
}

TEST_F(RedisClientCodecTest, Encode) {
  redis::Request r;
  r.params_.push_back("SET");
  r.params_.push_back("trpc");
  r.params_.push_back("support redis");
  ClientContextPtr ctx = MakeRefCounted<ClientContext>();
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();
  auto* redis_req_protocol = static_cast<RedisRequestProtocol*>(req_protocol.get());
  redis_req_protocol->redis_req = std::move(r);
  NoncontiguousBuffer buff;
  EXPECT_TRUE(codec_.ZeroCopyEncode(ctx, req_protocol, buff));
  trpc::BufferPtr rr = MakeRefCounted<Buffer>(buff.ByteSize());
  trpc::FlattenToSlow(buff, rr->GetWritePtr(), rr->WritableSize());
  rr->AddWriteLen(buff.ByteSize());
  EXPECT_EQ(
      "*3\r\n$3\r\nSET\r\n"
      "$4\r\ntrpc\r\n"
      "$13\r\nsupport redis\r\n",
      std::string(rr->GetReadPtr(), rr->ReadableSize()));
}

TEST_F(RedisClientCodecTest, EncodeWithContext) {
  redis::Request r;
  r.params_.push_back("SET");
  r.params_.push_back("trpc");
  r.params_.push_back("support redis");

  ClientContextPtr ctx = MakeRefCounted<ClientContext>();

  ProtocolPtr req_protocol = codec_.CreateRequestPtr();
  EXPECT_TRUE(codec_.FillRequest(ctx, req_protocol, reinterpret_cast<void*>(&r)));
  NoncontiguousBuffer buff;
  EXPECT_TRUE(codec_.ZeroCopyEncode(ctx, req_protocol, buff));
  trpc::BufferPtr rr = MakeRefCounted<Buffer>(buff.ByteSize());
  trpc::FlattenToSlow(buff, rr->GetWritePtr(), rr->WritableSize());
  rr->AddWriteLen(buff.ByteSize());
  EXPECT_EQ(
      "*3\r\n$3\r\nSET\r\n"
      "$4\r\ntrpc\r\n"
      "$13\r\nsupport redis\r\n",
      std::string(rr->GetReadPtr(), rr->ReadableSize()));
}

TEST_F(RedisClientCodecTest, Check) {
  std::deque<std::any> out;
  NoncontiguousBufferBuilder builder;
  NoncontiguousBuffer in = builder.DestructiveGet();
  auto result = codec_.ZeroCopyCheck(nullptr, in, out);
  EXPECT_EQ(0, static_cast<int32_t>(result));
}

}  // namespace testing

}  // namespace trpc
