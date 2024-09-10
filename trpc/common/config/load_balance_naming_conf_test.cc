//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the Apache 2.0 License.
// A copy of the Apache 2.0 License is included in this file.
//
#include "trpc/common/config/load_balance_naming_conf.h"
#include "trpc/common/config/load_balance_naming_conf_parser.h"

#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"
TEST(SWRoundrobinLoadBalanceConfig, load_test) {
  trpc::naming::SWRoundrobinLoadBalanceConfig config;

  config.services_weight["serviceA"] = {1, 2, 3};
  config.services_weight["serviceB"] = {4, 5, 6};
  config.Display();

  YAML::convert<trpc::naming::SWRoundrobinLoadBalanceConfig> c;
  YAML::Node config_node = c.encode(config);

  trpc::naming::SWRoundrobinLoadBalanceConfig tmp;
  ASSERT_TRUE(c.decode(config_node, tmp));

  tmp.Display();

  // Check that the original and decoded configurations have the same number of services
  ASSERT_EQ(config.services_weight.size(), tmp.services_weight.size());

  // Verify that each service has the same weights in the original and decoded configurations
  for (const auto& [service_name, weights] : config.services_weight) {
    ASSERT_EQ(tmp.services_weight[service_name].size(), weights.size());

    for (size_t i = 0; i < weights.size(); ++i) {
      ASSERT_EQ(tmp.services_weight[service_name][i], weights[i]);
    }
  }
}
