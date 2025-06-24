
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

#include "trpc/naming/direct/direct_selector_conf.h"

#include "gtest/gtest.h"

#include "trpc/naming/direct/direct_selector_conf_parser.h"

namespace trpc::naming::testing {

TEST(DirectSelectorConfigTest, parse) {
  DirectSelectorConfig direct_selector_config;
  direct_selector_config.circuit_break_config.plugin_name = "self";
  direct_selector_config.circuit_break_config.enable = false;
  direct_selector_config.circuit_break_config.plugin_config["enable"] = "false";

  YAML::Node root = YAML::convert<DirectSelectorConfig>::encode(direct_selector_config);
  DirectSelectorConfig tmp;
  YAML::convert<DirectSelectorConfig>::decode(root, tmp);
  ASSERT_TRUE(tmp.circuit_break_config.plugin_name == direct_selector_config.circuit_break_config.plugin_name);
  ASSERT_TRUE(tmp.circuit_break_config.enable == direct_selector_config.circuit_break_config.enable);
}

}  // namespace trpc::naming::testing