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

#include "trpc/tvar/compound_ops/internal_latency.h"

#include <string>

#include "gtest/gtest.h"

namespace {

using trpc::tvar::internal::InternalLatencyInTsc;
using trpc::tvar::internal::TscToDuration;

}  // namespace

namespace trpc::testing {

/// @brief Test for common usage.
TEST(InternalLatencyInTscTest, Common) {
  ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/series.yaml"), 0);
  InternalLatencyInTsc in_tsc("in");
  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  InternalLatencyInTsc in_tsc2(parent, "in2");
  ASSERT_TRUE(in_tsc2.IsExposed());

  for (int i = 0; i < 100; ++i) {
    in_tsc.Update(i);
    in_tsc2.Update(i);
  }

  auto jv = trpc::tvar::TrpcVarGroup::TryGet("/in");
  ASSERT_TRUE(jv->isObject());
  ASSERT_TRUE(jv->isMember("total"));
  ASSERT_EQ((*jv)["total"]["cnt"].asInt(), 100);
  ASSERT_EQ((*jv)["total"]["avg"].asInt(), TscToDuration()(49));

  auto jv2 = trpc::tvar::TrpcVarGroup::TryGet("/a/b/in2");
  ASSERT_TRUE(jv2->isObject());
  ASSERT_TRUE(jv2->isMember("total"));
  ASSERT_EQ((*jv2)["total"]["cnt"].asInt(), 100);
  ASSERT_EQ((*jv2)["total"]["avg"].asInt(), TscToDuration()(49));

  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  InternalLatencyInTsc repetitive_in_tsc2(parent, "in2");
  ASSERT_FALSE(repetitive_in_tsc2.IsExposed());
}

}  // namespace trpc::testing
