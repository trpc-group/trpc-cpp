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

#include "trpc/codec/server_codec.h"

#include <any>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/codec/testing/server_codec_testing.h"
#include "trpc/server/server_context.h"

namespace trpc::testing {

class ServerCodecTest : public ::testing::Test {
 protected:
  void SetUp() override { codec_ = std::make_shared<TestServerCodec>(); }

  void TearDown() override {}

 protected:
  std::shared_ptr<TestServerCodec> codec_;
};

TEST_F(ServerCodecTest, Name) { ASSERT_EQ("TestServerCodec", codec_->Name()); }

TEST_F(ServerCodecTest, ZeroCopyCheckReturnMinusOne) {
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  ASSERT_EQ(-1, codec_->ZeroCopyCheck(nullptr, in, out));
}

TEST_F(ServerCodecTest, ZeroCopyDecodeReturnFalse) {
  NoncontiguousBuffer in;
  auto out = codec_->CreateRequestObject();
  ASSERT_FALSE(codec_->ZeroCopyDecode(nullptr, in, out));
}

TEST_F(ServerCodecTest, ZeroCopyEncodeReturnFalse) {
  auto in = codec_->CreateResponseObject();
  NoncontiguousBuffer out;
  ASSERT_FALSE(codec_->ZeroCopyEncode(nullptr, in, out));
}

TEST_F(ServerCodecTest, CreateRequestObjectReturnNullptr) {
  ASSERT_NE(nullptr, codec_->CreateRequestObject());
}

TEST_F(ServerCodecTest, CreateResponseObjectReturnNullptr) {
  ASSERT_NE(nullptr, codec_->CreateResponseObject());
}

TEST_F(ServerCodecTest, PickReturnFalse) {
  std::any message{};
  std::any data{};
  ASSERT_FALSE(codec_->Pick(message, data));
}

TEST_F(ServerCodecTest, HasStreamingRpcReturnFalse) { ASSERT_FALSE(codec_->HasStreamingRpc()); }

TEST_F(ServerCodecTest, IsStreamingProtocolReturnFalse) { ASSERT_FALSE(codec_->HasStreamingRpc()); }

TEST_F(ServerCodecTest, GetProtocolRetCodeWithOk) {
  auto ret_code = codec::ServerRetCode::SUCCESS;
  ASSERT_EQ(0, codec_->GetProtocolRetCode(ret_code));
}

}  // namespace trpc::testing
