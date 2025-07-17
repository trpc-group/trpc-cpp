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

#include "trpc/overload_control/high_percentile/high_percentile_conf.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(HighPercentileOverloadControlConfTest, HighPercentileConf) {
  HighPercentileConf original_conf;
  HighPercentileConf decoded_conf;

  original_conf.max_priority = 1024;
  original_conf.max_schedule_delay = 5;
  original_conf.max_request_latency = 6;
  original_conf.dry_run = true;
  original_conf.is_report = false;
  original_conf.lower_step = 0.08;
  original_conf.upper_step = 0.09;
  original_conf.fuzzy_ratio = 0.05;
  original_conf.max_update_interval = 1000;
  original_conf.max_update_size = 1024;
  original_conf.histograms = 5;
  original_conf.min_concurrency_window_size = 1000;
  original_conf.min_max_concurrency = 50;

  YAML::Node node = YAML::convert<HighPercentileConf>::encode(original_conf);
  ASSERT_TRUE(YAML::convert<HighPercentileConf>::decode(node, decoded_conf));

  ASSERT_EQ(original_conf.max_priority, decoded_conf.max_priority);
  ASSERT_EQ(original_conf.max_schedule_delay, decoded_conf.max_schedule_delay);
  ASSERT_EQ(original_conf.max_request_latency, decoded_conf.max_request_latency);
  ASSERT_EQ(original_conf.dry_run, decoded_conf.dry_run);
  ASSERT_EQ(original_conf.is_report, decoded_conf.is_report);
  ASSERT_EQ(original_conf.lower_step, decoded_conf.lower_step);
  ASSERT_EQ(original_conf.upper_step, decoded_conf.upper_step);
  ASSERT_EQ(original_conf.fuzzy_ratio, decoded_conf.fuzzy_ratio);
  ASSERT_EQ(original_conf.max_update_interval, decoded_conf.max_update_interval);
  ASSERT_EQ(original_conf.max_update_size, decoded_conf.max_update_size);
  ASSERT_EQ(original_conf.histograms, decoded_conf.histograms);
  ASSERT_EQ(original_conf.min_concurrency_window_size, decoded_conf.min_concurrency_window_size);
  ASSERT_EQ(original_conf.min_max_concurrency, decoded_conf.min_max_concurrency);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
