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

#include "trpc/overload_control/high_percentile/high_percentile_priority_impl.h"

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(HighPercentileOverloadControlTest, MustOnRequest) {
  auto options = HighPercentilePriorityImpl::Options{
      .service_func = "Greeter/SayHello",
      .is_report = false,
      .strategies = {nullptr},
      .min_concurrency_window_size = 200,
      .min_max_concurrency = 10,
  };

  HighPercentilePriorityImpl high_percentile(options);
  bool ret = high_percentile.MustOnRequest();
  ASSERT_TRUE(ret);
  high_percentile.OnError();
  ret = high_percentile.MustOnRequest();
  ASSERT_TRUE(ret);
}

TEST(HighPercentileOverloadControlTest, OnRequest) {
  auto options = HighPercentilePriorityImpl::Options{
      .service_func = "Greeter/SayHello",
      .is_report = false,
      .strategies = {nullptr},
      .min_concurrency_window_size = 200,
      .min_max_concurrency = 10,
  };

  HighPercentilePriorityImpl high_percentile(options);
  bool ret = high_percentile.MustOnRequest();
  ASSERT_TRUE(ret);
}

TEST(HighPercentileOverloadControlTest, OnSuccess) {
  HighAvgStrategy mock_strategy(HighAvgStrategy::Options{
      .name = "mock",
      .expected = 1.0,
  });

  HighPercentilePriorityImpl high_percentile(HighPercentilePriorityImpl::Options{
      .service_func = "Greeter/SayHello",
      .is_report = false,
      .strategies = {&mock_strategy},
      .min_concurrency_window_size = 200,
      .min_max_concurrency = 10,
  });

  for (int i = 0; i < 10000; ++i) {
    mock_strategy.Sample(10.0);
    if (i % 5 == 0) {
      mock_strategy.GetMeasured();  // Trigger advance
    }
  }

  ASSERT_GT(mock_strategy.GetMeasured(), 1.0);
  ASSERT_GT(mock_strategy.GetMeasured() / mock_strategy.GetExpected(), 1);

  int successes = 0;
  int fails = 0;

  for (int i = 0; i < 10000; ++i) {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    mock_strategy.Sample(10.0);

    ASSERT_GT(mock_strategy.GetMeasured(), 1.0);
    ASSERT_GT(mock_strategy.GetMeasured() / mock_strategy.GetExpected(), 1);

    if (high_percentile.OnRequest() && i % 2 == 1) {
      high_percentile.OnSuccess(std::chrono::milliseconds(5));
      ++successes;
    } else {
      ++fails;
    }
  }

  ASSERT_NE(successes, 0);
  ASSERT_NE(fails, 0);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
