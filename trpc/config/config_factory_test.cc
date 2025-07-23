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

#include <any>
#include <map>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/config/testing/config_plugin_testing.h"

namespace trpc::testing {

TEST(ConfigFactory, TestRegistry) {
  TestConfigPluginPtr p = MakeRefCounted<TestConfigPlugin>();
  ConfigFactory::GetInstance()->Register(p);

  // Verify that the registration was successful
  ConfigPtr config_ptr = ConfigFactory::GetInstance()->Get("TestConfigPlugin");
  EXPECT_EQ(p.get(), config_ptr.get());
}

}  // namespace trpc::testing
