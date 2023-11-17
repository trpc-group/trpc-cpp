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

#include "trpc/common/plugin.h"

#include <limits>
#include <string>

#include "gtest/gtest.h"

namespace trpc::testing {

class TestPlugin1 : public Plugin {
 public:
  TestPlugin1() = default;

  PluginType Type() const override { return PluginType::kUnspecified; }

  std::string Name() const override { return "TestPlugin1"; }
};

class TestPlugin2 : public Plugin {
 public:
  TestPlugin2() = default;

  PluginType Type() const override { return PluginType::kUnspecified; }

  std::string Name() const override { return "TestPlugin2"; }
};

TEST(PluginTest, TestPluginID) {
  TestPlugin1 plugin1;
  TestPlugin2 plugin2;

  uint32_t plugin1_id = plugin1.GetPluginID();
  uint32_t plugin2_id = plugin2.GetPluginID();

  EXPECT_EQ(plugin1_id + 1, plugin2_id);

  uint32_t expected_plugin1_id = std::numeric_limits<uint16_t>::max(); // 65535
  uint32_t expected_plugin2_id = std::numeric_limits<uint16_t>::max() + 1; // 65536

  EXPECT_EQ(plugin1_id, expected_plugin1_id);
  EXPECT_EQ(plugin2_id, expected_plugin2_id);
}

}  // namespace trpc::testing
