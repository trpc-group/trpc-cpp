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

#include "trpc/compressor/compressor_factory.h"

#include "gtest/gtest.h"

#include "trpc/compressor/testing/compressor_testing.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::compressor::testing {

TEST(CompressorFactory, RegistryFalse) {
  auto p =  MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*p, Type()).WillOnce(::testing::Return(kMaxType));
  ASSERT_FALSE(CompressorFactory::GetInstance()->Register(p));
}

TEST(CompressorFactory, GetWithoutRegistry) {
  ASSERT_FALSE(CompressorFactory::GetInstance()->Get(kGzip));
  ASSERT_FALSE(CompressorFactory::GetInstance()->Get(kMaxType));
}

TEST(CompressorFactory, CompressorFactoryTest) {
  CompressType type{128};
  auto p = MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*p, Type()).WillOnce(::testing::Return(type));

  CompressorFactory::GetInstance()->Clear();
  CompressorFactory::GetInstance()->Register(p);

  auto ret = CompressorFactory::GetInstance()->Get(type);
  ASSERT_EQ(p.get(), ret);

  CompressorFactory::GetInstance()->Clear();

  auto empty = CompressorFactory::GetInstance()->Get(type);
  ASSERT_EQ(empty, nullptr);
}

}  // namespace trpc::compressor::testing
