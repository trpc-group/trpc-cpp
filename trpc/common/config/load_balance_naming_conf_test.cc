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

#include "trpc/common/config/load_balance_naming_conf.h"
#include "trpc/common/config/load_balance_naming_conf_parser.h"

#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"
TEST(SWRoundrobinLoadBalanceConfig, load_test) {
  // Create an instance of SWRoundrobinLoadBalanceConfig with multiple services
  trpc::naming::SWRoundrobinLoadBalanceConfig config;

  config.services["serviceA"] = {
      {"127.0.0.1:10000", 5},
      {"127.0.0.1:10003", 2},
  };

  config.services["serviceB"] = {
      {"127.0.0.2:20000", 3},
      {"127.0.0.2:20003", 4},
  };

  config.Display();

  YAML::convert<trpc::naming::SWRoundrobinLoadBalanceConfig> c;
  YAML::Node config_node = c.encode(config);

  // Decode the YAML back to a SWRoundrobinLoadBalanceConfig object
  trpc::naming::SWRoundrobinLoadBalanceConfig tmp;
  ASSERT_TRUE(c.decode(config_node, tmp));

  tmp.Display();

  // Check that the original and decoded configurations are equal
  ASSERT_EQ(config.services.size(), tmp.services.size());

  for (const auto& [service_name, address_weight] : config.services) {
    // Check the address-weight mapping for each service
    ASSERT_EQ(tmp.services[service_name].size(), address_weight.size());

    for (const auto& [address, weight] : address_weight) {
      ASSERT_EQ(tmp.services[service_name][address], weight);
    }
  }
}
