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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/high_percentile/high_avg_ema.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(HighAvgEMA, EMATestLarge) {
  HighAvgEMA ema(HighAvgEMA::Options{
      .inc_factor = 1.1,
      .dec_factor = -0.1,
  });

  ASSERT_DOUBLE_EQ(ema.Get(), 0.0);

  ema.UpdateAndGet(2.0);
  ASSERT_DOUBLE_EQ(ema.Get(), 2.0);

  ema.UpdateAndGet(1);
  ASSERT_DOUBLE_EQ(ema.Get(), 2.0);
}

TEST(HighAvgEMA, EMATestSmall) {
  HighAvgEMA ema(HighAvgEMA::Options{
      .inc_factor = -0.1,
      .dec_factor = 1.1,
  });

  ASSERT_DOUBLE_EQ(ema.Get(), 0.0);

  ema.UpdateAndGet(-2.0);
  ASSERT_DOUBLE_EQ(ema.Get(), -2.0);

  ema.UpdateAndGet(-1.0);
  ASSERT_DOUBLE_EQ(ema.Get(), -2.0);
}

TEST(HighAvgEMA, EMATestRandom) {
  HighAvgEMA ema(HighAvgEMA::Options{
      .inc_factor = 0.1,
      .dec_factor = 0.001,
  });

  ASSERT_DOUBLE_EQ(ema.Get(), 0.0);

  ema.UpdateAndGet(1);
  ASSERT_DOUBLE_EQ(ema.Get(), 0.1);

  ema.UpdateAndGet(-1);
  ASSERT_DOUBLE_EQ(ema.Get(), 0.0989);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
