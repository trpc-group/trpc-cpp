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

#include "trpc/overload_control/throttler/throttler_conf.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(TestThrottlerOverloadControlConf, ThrottlerConf) {
  ThrottlerConf original_conf;
  ThrottlerConf decoded_conf;

  original_conf.max_priority = 1024;
  original_conf.dry_run = true;
  original_conf.is_report = false;
  original_conf.lower_step = 0.08;
  original_conf.upper_step = 0.09;
  original_conf.fuzzy_ratio = 0.05;
  original_conf.max_update_interval = 1000;
  original_conf.max_update_size = 1024;
  original_conf.histograms = 5;
  original_conf.ratio_for_accepts = 2;
  original_conf.requests_padding = 10;
  original_conf.max_throttle_probability = 0.9;
  original_conf.ema_factor = 0.75;
  original_conf.ema_interval_ms = 100;
  original_conf.ema_reset_interval_ms = 1000;

  YAML::Node node = YAML::convert<ThrottlerConf>::encode(original_conf);
  ASSERT_TRUE(YAML::convert<ThrottlerConf>::decode(node, decoded_conf));

  ASSERT_EQ(original_conf.max_priority, decoded_conf.max_priority);
  ASSERT_EQ(original_conf.dry_run, decoded_conf.dry_run);
  ASSERT_EQ(original_conf.is_report, decoded_conf.is_report);
  ASSERT_EQ(original_conf.lower_step, decoded_conf.lower_step);
  ASSERT_EQ(original_conf.upper_step, decoded_conf.upper_step);
  ASSERT_EQ(original_conf.fuzzy_ratio, decoded_conf.fuzzy_ratio);
  ASSERT_EQ(original_conf.max_update_interval, decoded_conf.max_update_interval);
  ASSERT_EQ(original_conf.max_update_size, decoded_conf.max_update_size);
  ASSERT_EQ(original_conf.histograms, decoded_conf.histograms);
  ASSERT_EQ(original_conf.ratio_for_accepts, decoded_conf.ratio_for_accepts);
  ASSERT_EQ(original_conf.requests_padding, decoded_conf.requests_padding);
  ASSERT_EQ(original_conf.max_throttle_probability, decoded_conf.max_throttle_probability);
  ASSERT_EQ(original_conf.ema_factor, decoded_conf.ema_factor);
  ASSERT_EQ(original_conf.ema_interval_ms, decoded_conf.ema_interval_ms);
  ASSERT_EQ(original_conf.ema_reset_interval_ms, decoded_conf.ema_reset_interval_ms);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
