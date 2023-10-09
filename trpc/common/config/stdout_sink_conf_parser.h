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

#include "trpc/common/config/default_log_conf_parser.h"

namespace YAML {

bool GetStdoutNode(std::string_view logger_name, YAML::Node& stdout_node) {
  YAML::Node logger_node, sinks_node;
  // Check if logger is configured
  if (!GetLoggerNode(logger_name, logger_node)) {
    std::cerr << "Get loggerNode err: " << "logger_name" << logger_name << std::endl;
    return false;
  }
  // Check if sinks are configured
  if (!trpc::ConfigHelper::GetNode(logger_node, {"sinks"}, sinks_node)) {
    std::cerr << "sink not found!" << std::endl;
    return false;
  }
  // Check if local_file is configured
  if (!trpc::ConfigHelper::GetNode(sinks_node, {"stdout"}, stdout_node)) {
    return false;
  }
  return true;
}

template <>
struct convert<trpc::StdoutSinkConfig> {
  static YAML::Node encode(const trpc::StdoutSinkConfig& config) {
    YAML::Node node;
    node["format"] = config.format;
    node["eol"] = config.eol;
    node["stderr_level"] = config.stderr_level;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::StdoutSinkConfig& config) {
    if (node["format"]) {
      config.format = node["format"].as<std::string>();
    }
    if (node["eol"]) {
      config.eol = node["eol"].as<bool>();
    }
    if (node["stderr_level"]) {
      config.stderr_level = node["stderr_level"].as<unsigned int>();
    }
    return true;
  }
};

}  // namespace YAML
