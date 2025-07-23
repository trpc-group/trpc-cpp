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

#include "trpc/config/codec.h"

namespace trpc::config {

class TestCodecPlugin : public Codec {
 public:
  std::string Name() const override { return "TestCodecPlugin"; }

  bool Decode(const std::string& data, std::unordered_map<std::string, std::any>& out) const override {
    // Dummy decode method, always return true
    return true;
  }
};

using TestCodecPluginPtr = RefPtr<TestCodecPlugin>;

}  // namespace trpc::config
