
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

#include "yaml-cpp/yaml.h"

#include "trpc/naming/direct/direct_selector_conf.h"

namespace YAML {

template <>
struct convert<trpc::naming::DirectSelectorConfig> {
  static YAML::Node encode(const trpc::naming::DirectSelectorConfig& config) {
    YAML::Node node;

    node["circuitBreaker"] = config.circuit_break_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::DirectSelectorConfig& config) {
    if (node["circuitBreaker"]) {
      config.circuit_break_config = node["circuitBreaker"].as<trpc::naming::CircuitBreakConfig>();
    }

    return true;
  }
};

}  // namespace YAML
