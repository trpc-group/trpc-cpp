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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/high_percentile/high_avg_strategy.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(HighAvgStrategyTest, All) {
  HighAvgStrategy strategy(HighAvgStrategy::Options{
      .name = "test",
      .expected = 1.0,
  });

  ASSERT_EQ(strategy.Name(), "test");
  ASSERT_EQ(strategy.GetExpected(), 1.0);
  ASSERT_EQ(strategy.GetMeasured(), 0.0);

  for (int i = 0; i < 1000; ++i) {
    strategy.Sample(i % 10);
    if (i % 100 == 0) {
      strategy.GetMeasured();  // Trigger advance
    }
  }
  ASSERT_NE(strategy.GetMeasured(), 0.0);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
