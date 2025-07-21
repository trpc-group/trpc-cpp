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

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/retry_conf.h"

namespace YAML {

template <>
struct convert<trpc::RetryHedgingLimitConfig> {
  static YAML::Node encode(const trpc::RetryHedgingLimitConfig& config) {
    YAML::Node node;
    node["max_tokens"] = config.max_tokens;
    node["token_ratio"] = config.token_ratio;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::RetryHedgingLimitConfig& config) {  // NOLINT
    if (node["max_tokens"]) {
      config.max_tokens = node["max_tokens"].as<int>();
    }
    if (node["token_ratio"]) {
      config.token_ratio = node["token_ratio"].as<int>();
    }
    return true;
  }
};

}  // namespace YAML
