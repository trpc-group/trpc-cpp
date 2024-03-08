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

#include "trpc/naming/common/util/circuit_break/default_circuit_breaker_config.h"

#include "gtest/gtest.h"

namespace trpc::naming::testing {

TEST(DefaultCircuitBreakerConfig, Parse) {
  std::string config_str =
      "enable: false\r\n"
      "statWindow: 30s\r\n"
      "bucketsNum: 6\r\n"
      "sleepWindow: 10s\r\n"
      "requestVolumeThreshold: 5\r\n"
      "errorRateThreshold: 0.4\r\n"
      "continuousErrorThreshold: 5\r\n"
      "requestCountAfterHalfOpen: 5\r\n"
      "successCountAfterHalfOpen: 4";

  YAML::Node plugin_config = YAML::Load(config_str);

  DefaultCircuitBreakerConfig circuit_breaker_config;
  YAML::convert<DefaultCircuitBreakerConfig>::decode(plugin_config, circuit_breaker_config);
  ASSERT_EQ(circuit_breaker_config.enable, false);
  ASSERT_EQ(circuit_breaker_config.stat_window_ms, 30000);
  ASSERT_EQ(circuit_breaker_config.buckets_num, 6);
  ASSERT_EQ(circuit_breaker_config.sleep_window_ms, 10000);
  ASSERT_EQ(circuit_breaker_config.request_volume_threshold, 5);
  ASSERT_EQ(circuit_breaker_config.error_rate_threshold, 0.4f);
  ASSERT_EQ(circuit_breaker_config.continuous_error_threshold, 5);
  ASSERT_EQ(circuit_breaker_config.request_count_after_half_open, 5);
  ASSERT_EQ(circuit_breaker_config.success_count_after_half_open, 4);
}

}  // namespace trpc::naming::testing
