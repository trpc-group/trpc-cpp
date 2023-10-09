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

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/config_helper.h"
#include "trpc/common/config/default_log_conf.h"

namespace YAML {

/// @brief Get yaml configuration under defalut plugin.
bool GetDefaultLogNode(YAML::Node& default_log_node);

/// @brief Get the yaml configuration under the logger.
bool GetLoggerNode(std::string_view logger_name, YAML::Node& logger_node);

template <>
struct convert<trpc::DefaultLogConfig::LoggerInstance> {
  static YAML::Node encode(const trpc::DefaultLogConfig::LoggerInstance& config) {
    YAML::Node node;

    node["name"] = config.name;
    node["min_level"] = config.min_level;
    node["format"] = config.format;
    node["mode"] = config.mode;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::DefaultLogConfig::LoggerInstance& config) {
    if (node["name"]) {
      config.name = node["name"].as<std::string>();
    }

    if (node["min_level"]) {
      config.min_level = node["min_level"].as<unsigned int>();
    }

    if (node["format"]) {
      config.format = node["format"].as<std::string>();
    }

    if (node["mode"]) {
      config.mode = node["mode"].as<unsigned int>();
    }

    return true;
  }
};

template <>
struct convert<trpc::DefaultLogConfig> {
  static YAML::Node encode(const trpc::DefaultLogConfig& config) {
    YAML::Node node;
    node = config.instances;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::DefaultLogConfig& config) {
    for (size_t idx = 0; idx < node.size(); ++idx) {
      config.instances.push_back(node[idx].as<trpc::DefaultLogConfig::LoggerInstance>());
    }
    return true;
  }
};

}  // namespace YAML
