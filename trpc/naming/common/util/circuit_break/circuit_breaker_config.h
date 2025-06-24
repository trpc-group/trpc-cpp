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

#include <string>

#include "yaml-cpp/yaml.h"

#include "trpc/util/log/logging.h"

namespace trpc::naming {

struct CircuitBreakConfig {
  std::string plugin_name{"default"};  // circuit break plugin name
  bool enable{false};                  // enable circuit break or not
  YAML::Node plugin_config;            // config of circuit break plugin

  void Display() const {
    TRPC_LOG_DEBUG("----------CircuitBreakConfig----------");
    TRPC_LOG_DEBUG("plugin_name:" << plugin_name);
    TRPC_LOG_DEBUG("enable:" << (enable ? "true" : "false"));
    TRPC_LOG_DEBUG("plugin_config:" << plugin_config);
  }
};

}  // namespace trpc::naming

namespace YAML {

template <>
struct convert<trpc::naming::CircuitBreakConfig> {
  static YAML::Node encode(const trpc::naming::CircuitBreakConfig& config) {
    YAML::Node node;

    node[config.plugin_name] = config.plugin_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::CircuitBreakConfig& config) {
    auto iter = node.begin();
    if (iter != node.end()) {
      config.plugin_name = iter->first.as<std::string>();
      config.plugin_config = iter->second;

      if (iter->second) {
        if (iter->second["enable"]) {
          config.enable = iter->second["enable"].as<bool>();
        }
      }
    }

    return true;
  }
};

}  // namespace YAML
