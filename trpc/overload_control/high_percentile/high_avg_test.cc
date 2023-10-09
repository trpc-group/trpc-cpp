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

#include "trpc/overload_control/high_percentile/high_avg.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/logging/trpc_logging.h"

namespace trpc::overload_control {
namespace testing {

TEST(HighAvgTest, All) {
  std::vector<double> doubles(10000);
  // {
  //   0.0000, 0.1000, 0.2000, ... ,0.9000,
  //   0.0001, 0.1001, 0.2001, ... ,0.9001,
  //   0.0002, 0.1002, 0.2002, ... ,0.9002,
  //   ......
  //   0.0999, 0.1999, 0.2999, ... ,0.9999,
  // }
  for (int i = 0; i < 1000; ++i) {
    for (int j = 0; j < 10; ++j) {
      doubles[10 * i + j] = 0.1 * j + 0.0001 * i;
    }
  }

  HighAvg high_avg;

  for (int i = 0; i < 10000; ++i) {
    high_avg.Sample(doubles[i]);
    if (i % 100 == 0) {
      high_avg.AdvanceAndGet();  // do advance
    }
  }

  std::sort(doubles.begin(), doubles.end());

  double estimate_high_p = high_avg.AdvanceAndGet();
  double real_high_p = doubles[10000 * 97 / 100];

  TRPC_FMT_INFO("estimate_high_p: {}, real_high_p: {}", estimate_high_p, real_high_p);
  ASSERT_GT(estimate_high_p, real_high_p);
  ASSERT_LT(std::abs((estimate_high_p - real_high_p)) / real_high_p, 0.1);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
