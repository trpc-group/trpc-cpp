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

#include "trpc/codec/codec_manager.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(CodecManagerTest, Init) {
  EXPECT_TRUE(codec::Init());

  EXPECT_TRUE(ClientCodecFactory::GetInstance()->Get("http"));
  EXPECT_TRUE(ServerCodecFactory::GetInstance()->Get("http"));

  EXPECT_TRUE(ClientCodecFactory::GetInstance()->Get("trpc"));
  EXPECT_TRUE(ServerCodecFactory::GetInstance()->Get("trpc"));

  EXPECT_TRUE(ClientCodecFactory::GetInstance()->Get("redis"));

  EXPECT_TRUE(ClientCodecFactory::GetInstance()->Get("grpc"));
  EXPECT_TRUE(ServerCodecFactory::GetInstance()->Get("grpc"));
}

TEST(CodecManagerTest, Destroy) {
  EXPECT_TRUE(codec::Init());

  EXPECT_TRUE(ClientCodecFactory::GetInstance()->Get("http"));
  EXPECT_TRUE(ServerCodecFactory::GetInstance()->Get("http"));

  codec::Destroy();

  EXPECT_FALSE(ClientCodecFactory::GetInstance()->Get("http"));
  EXPECT_FALSE(ServerCodecFactory::GetInstance()->Get("http"));
}

}  // namespace trpc::testing
