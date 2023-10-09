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

#include "trpc/codec/server_codec_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/server_codec_testing.h"

namespace trpc::testing {

TEST(ServerCodecFactory, ServerCodecFactoryRegister) {
  ServerCodecPtr p(new TestServerCodec());
  ServerCodecFactory::GetInstance()->Register(p);

  ServerCodecPtr ret = ServerCodecFactory::GetInstance()->Get("TestServerCodec");
  EXPECT_EQ(p.get(), ret.get());
}

TEST(ServerCodecFactory, ServerCodecFactoryGet) {
  ServerCodecPtr ret = ServerCodecFactory::GetInstance()->Get("TestServerCodec");
  EXPECT_EQ(true, ret != nullptr);
}

TEST(ServerCodecFactory, ServerCodecFactoryGetNullptr) {
  ServerCodecPtr ret = ServerCodecFactory::GetInstance()->Get("TestServerCodec1");
  EXPECT_EQ(true, ret == nullptr);
}

TEST(ServerCodecFactory, Clear) {
  ServerCodecPtr p(new TestServerCodec());
  ServerCodecFactory::GetInstance()->Register(p);

  ServerCodecPtr ret = ServerCodecFactory::GetInstance()->Get("TestServerCodec");
  EXPECT_NE(p.get(), ret.get());

  ServerCodecFactory::GetInstance()->Clear();
  ServerCodecPtr ret2 = ServerCodecFactory::GetInstance()->Get("TestServerCodec");
  EXPECT_EQ(true, ret2 == nullptr);
}
}  // namespace trpc::testing
