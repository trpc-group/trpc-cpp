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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "trpc/config/provider.h"

namespace trpc::config {

class TestProviderPlugin : public Provider {
 public:
  std::string Name() const override { return "TestProviderPlugin"; }

  std::string Read(const std::string&) override {
    // Dummy read method, always return empty content
    return "";
  }

  void Watch(ProviderCallback) override {
    // Dummy watch method, does nothing
  }
};

using TestProviderPluginPtr = RefPtr<TestProviderPlugin>;

}  // namespace trpc::config
