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

#include "trpc/overload_control/common/histogram.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(HistogramTest, All) {
  Histogram histogram(Histogram::Options{
      .priorities = 2,
  });
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(1, 0), 1.0, 0.01);

  // #0 == #1 == 100, total = 200
  for (int i = 0; i < 1000; ++i) {
    histogram.Sample(0);
    histogram.Sample(1);
  }

  ASSERT_GT(histogram.Total(), 1000);

  ASSERT_NEAR(histogram.ShiftFuzzyPriority(0, -0.1), 0.0, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(2, 0.1), 2.0, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(0.5, 0.1), 0.7, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(0.5, -0.1), 0.3, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(1.5, 0.2), 1.9, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(1.5, -0.2), 1.1, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(0.9, 0.1), 1.1, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(1.1, -0.1), 0.9, 0.01);

  // #0 == 3 * #1 == 300, total = 400
  histogram.Reset();
  for (int i = 0; i < 100; ++i) {
    histogram.Sample(0);
    histogram.Sample(0);
    histogram.Sample(0);
    histogram.Sample(1);
  }

  ASSERT_NEAR(histogram.ShiftFuzzyPriority(0.9, 0.2), 1.5, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(1.2, -0.2), 0.8, 0.01);

  // #0 == #1 == 1, total = 2
  histogram.Reset();
  histogram.Sample(0);
  histogram.Sample(1);

  ASSERT_NEAR(histogram.ShiftFuzzyPriority(0.2, 0.1), 0.5, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(2, -0.1), 1.5, 0.01);
  ASSERT_NEAR(histogram.ShiftFuzzyPriority(1.2, -0.1), 0.5, 0.01);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
