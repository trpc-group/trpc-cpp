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

#include "trpc/codec/client_codec_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/codec/testing/client_codec_testing.h"

namespace trpc::testing {

TEST(ClientCodecFactory, ClientCodecFactoryRegister) {
  ClientCodecPtr p(new TestClientCodec());
  ClientCodecFactory::GetInstance()->Register(p);

  ClientCodecPtr ret = ClientCodecFactory::GetInstance()->Get("TestClientCodec");
  EXPECT_EQ(p.get(), ret.get());
}

TEST(ClientCodecFactory, ClientCodecFactoryGet) {
  ClientCodecPtr ret = ClientCodecFactory::GetInstance()->Get("TestClientCodec");
  EXPECT_EQ(true, ret != nullptr);
}

TEST(ClientCodecFactory, ClientCodecFactoryGetNullptr) {
  ClientCodecPtr ret = ClientCodecFactory::GetInstance()->Get("TestClientCodec1");
  EXPECT_EQ(true, ret == nullptr);
}

TEST(ClientCodecFactory, Clear) {
  ClientCodecPtr p(new TestClientCodec());
  ClientCodecFactory::GetInstance()->Register(p);

  ClientCodecPtr ret = ClientCodecFactory::GetInstance()->Get("TestClientCodec");
  EXPECT_NE(p.get(), ret.get());

  ClientCodecFactory::GetInstance()->Clear();
  ClientCodecPtr ret2 = ClientCodecFactory::GetInstance()->Get("TestClientCodec");
  EXPECT_EQ(true, ret2 == nullptr);
}

}  // namespace trpc::testing
