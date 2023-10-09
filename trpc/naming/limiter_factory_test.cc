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

#include "trpc/naming/limiter_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/naming/testing/trpc_limiter_testing.h"

namespace trpc::testing {

TEST(TestLimiter, TestLimiterFactory) {
  LimiterPtr p = MakeRefCounted<TestTrpcLimiter>();
  EXPECT_TRUE(p->Type() == PluginType::kLimiter);

  LimiterFactory::GetInstance()->Register(p);

  LimiterPtr ret = LimiterFactory::GetInstance()->Get(p->Name());
  EXPECT_EQ(p.get(), ret.get());

  LimiterFactory::GetInstance()->Clear();
  ret = LimiterFactory::GetInstance()->Get(p->Name());
  EXPECT_EQ(nullptr, ret.Get());
}

}  // namespace trpc::testing
