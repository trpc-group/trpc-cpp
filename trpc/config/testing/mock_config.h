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

#pragma once

#include "gmock/gmock.h"
#include "trpc/config/config.h"

namespace trpc::testing {

class MockConfig : public trpc::Config {
 public:
  MOCK_METHOD(bool, PullFileConfig,
              (const std::string& config_name, std::string* config, const std::any& params),
              (override));

  MOCK_METHOD(bool, PullKVConfig,
              (const std::string& config_name, const std::string& key, std::string* config,
               const std::any& params),
              (override));

  MOCK_METHOD(bool, PullMultiKVConfig,
              (const std::string&, (std::map<std::string, std::string>*), const std::any&), (override));

  MOCK_METHOD(void, SetAsyncCallBack, (const std::any& param), (override));

  PluginType Type() const override {
    return PluginType::kConfig;
  }

  std::string Name() const override {
    return "MockConfig";
  }
};

}  // namespace trpc::testing