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

#include <any>
#include <map>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/config/testing/provider_plugin_testing.h"

namespace trpc::testing {

class TestProviderPluginTest : public ::testing::Test {
 protected:
  void SetUp() override {
    provider_plugin_ = MakeRefCounted<config::TestProviderPlugin>();
    ASSERT_TRUE(provider_plugin_ != nullptr);
  }

  config::ProviderPtr provider_plugin_;
};

TEST_F(TestProviderPluginTest, BasicInformationCheck) {
  EXPECT_EQ(provider_plugin_->Type(), PluginType::kConfigProvider);
  EXPECT_EQ(provider_plugin_->Name(), "TestProviderPlugin");
}

TEST_F(TestProviderPluginTest, ReadMethod) {
  std::string dummy_path = "dummy_path_for_testing";
  std::string result_data = provider_plugin_->Read(dummy_path);
  // Check if the read function returns empty content as expected
  EXPECT_EQ(result_data, "");
}

TEST_F(TestProviderPluginTest, WatchMethod) {
  // Dummy callback function
  config::ProviderCallback callback = [](const std::string&, const std::string&) {};

  // Call Watch method, it should not cause any issue as it does nothing in the example
  provider_plugin_->Watch(callback);
}

}  // namespace trpc::testing
