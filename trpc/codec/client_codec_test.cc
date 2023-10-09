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

#include "trpc/codec/client_codec.h"

#include <any>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/codec/testing/client_codec_testing.h"


namespace trpc::testing {

class ClientCodecTest : public ::testing::Test {
 protected:
  void SetUp() override { codec_ = std::make_shared<TestClientCodec>(); }

  void TearDown() override {}

 protected:
  std::shared_ptr<TestClientCodec> codec_;
};

TEST_F(ClientCodecTest, Name) { EXPECT_EQ("TestClientCodec", codec_->Name()); }

TEST_F(ClientCodecTest, ZeroCopyCheckReturnMinusOne) {
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(-1, codec_->ZeroCopyCheck(nullptr, in, out));
}

TEST_F(ClientCodecTest, ZeroCopyDecodeReturnFalse) {
  NoncontiguousBuffer in;
  auto out = codec_->CreateResponsePtr();
  ASSERT_FALSE(codec_->ZeroCopyDecode(nullptr, in, out));
}

TEST_F(ClientCodecTest, ZeroCopyEncodeReturnFalse) {
  auto in = codec_->CreateRequestPtr();
  NoncontiguousBuffer out;
  ASSERT_FALSE(codec_->ZeroCopyEncode(nullptr, in, out));
}

TEST_F(ClientCodecTest, FillRequestOk) {
  //  auto context = MakeRefCounted<ClientContext>();
  //  auto request_protocol = codec_->CreateRequestPtr();
  EXPECT_TRUE(codec_->FillRequest(nullptr, nullptr, nullptr));
}

TEST_F(ClientCodecTest, FillResponseOk) {
  //  auto context = MakeRefCounted<ClientContext>();
  //  auto response_protocol = codec_->CreateResponsePtr();
  EXPECT_TRUE(codec_->FillResponse(nullptr, nullptr, nullptr));
}

TEST_F(ClientCodecTest, CreateRequestPtrReturnNullptr) {
  ASSERT_EQ(nullptr, codec_->CreateRequestPtr());
}

TEST_F(ClientCodecTest, CreateResponsePtrReturnNullptr) {
  ASSERT_EQ(nullptr, codec_->CreateResponsePtr());
}

TEST_F(ClientCodecTest, SequenceId) {
  ProtocolPtr rsp(new TestProtocol());
  EXPECT_EQ(codec_->GetSequenceId(rsp), -1);

  ProtocolPtr rsp_protocol = codec_->CreateResponsePtr();
  EXPECT_EQ(codec_->GetSequenceId(rsp_protocol), -1);
}

TEST_F(ClientCodecTest, PickReturnFalse) {
  std::any message{};
  std::any data{};
  ASSERT_FALSE(codec_->Pick(message, data));
}

TEST_F(ClientCodecTest, IsComplexReturnFalse) { EXPECT_FALSE(codec_->IsComplex()); }

TEST_F(ClientCodecTest, HasStreamingRpcReturnFalse) { ASSERT_FALSE(codec_->HasStreamingRpc()); }

TEST_F(ClientCodecTest, IsStreamingProtocolReturnFalse) { ASSERT_FALSE(codec_->HasStreamingRpc()); }

TEST_F(ClientCodecTest, GetProtocolRetCodeWithOk) {
  auto ret_code = codec::ClientRetCode::SUCCESS;
  ASSERT_EQ(0, codec_->GetProtocolRetCode(ret_code));
}

}  // namespace trpc::testing
