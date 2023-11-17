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
