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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/util/sampler.h"

#include <stdlib.h>
#include <time.h>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"

namespace trpc::testing {

/// @brief Test fixture to load config.
class TestHighLowWaterLevelSampler : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/rpcz/testing/rpcz.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test for HighLowWaterLevelSampler.
TEST_F(TestHighLowWaterLevelSampler, Sample) {
  trpc::rpcz::HighLowWaterLevelSampler sampler;

  ASSERT_EQ(sampler.GetName(), "HighLowWaterLevelSampler");

  sampler.Init();

  // Less than low water.
  ASSERT_TRUE(sampler.Sample(1));
  ASSERT_TRUE(sampler.Sample(2));
  ASSERT_TRUE(sampler.Sample(3));
  ASSERT_TRUE(sampler.Sample(4));
  ASSERT_TRUE(sampler.Sample(5));
  ASSERT_TRUE(sampler.Sample(6));
  ASSERT_TRUE(sampler.Sample(7));
  ASSERT_TRUE(sampler.Sample(8));
  ASSERT_TRUE(sampler.Sample(9));

  // Bigger than or equal to low water, and hit sample rate.
  ASSERT_TRUE(sampler.Sample(10));
  ASSERT_TRUE(sampler.Sample(12));
  ASSERT_TRUE(sampler.Sample(14));
  ASSERT_TRUE(sampler.Sample(16));
  ASSERT_TRUE(sampler.Sample(18));
  ASSERT_TRUE(sampler.Sample(20));

  // Bigger than or equal to low water, but not hit sample rate.
  ASSERT_FALSE(sampler.Sample(13));

  // Window sample count more than high water, stop sample.
  ASSERT_FALSE(sampler.Sample(21));
  ASSERT_FALSE(sampler.Sample(22));
  ASSERT_FALSE(sampler.Sample(23));
}

/// @brief Test for SampleRateLimiter.
TEST_F(TestHighLowWaterLevelSampler, IncreaseAndGetSampleNum) {
  trpc::rpcz::SampleRateLimiter sampler;

  sampler.Init();
  sampler.Increase();
  sampler.Increase();
  ASSERT_EQ(sampler.GetSampleNum(), 2);
}

}  // namespace trpc::testing
#endif
