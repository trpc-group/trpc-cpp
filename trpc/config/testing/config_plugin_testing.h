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

#pragma once

#include "trpc/config/config_factory.h"

namespace trpc::testing {

class TestConfigPlugin : public Config {
 public:
  TestConfigPlugin() = default;

  ~TestConfigPlugin() override = default;

  PluginType Type() const override { return PluginType::kConfig; }

  std::string Name() const override { return "TestConfigPlugin"; }

  std::string Version() const { return "0.0.1"; }

  int Init() noexcept override { return 0; }

  void Start() noexcept override {}

  void Stop() noexcept override {}

  bool PullFileConfig(const std::string& config_name, std::string* config, const std::any& params) override {
    return true;
  }

  bool PullKVConfig(const std::string& config_name, const std::string& key, std::string* config,
                    const std::any& params) override {
    return true;
  }

  bool PullMultiKVConfig(const std::string& config_name, std::map<std::string, std::string>* config,
                         const std::any& params) override {
    return true;
  }
};

using TestConfigPluginPtr = RefPtr<TestConfigPlugin>;

}  // namespace trpc::testing
