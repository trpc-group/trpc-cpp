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
struct convert<trpc::LocalFileSinkConfig> {
  static YAML::Node encode(const trpc::LocalFileSinkConfig& config) {
    YAML::Node node;
    node["format"] = config.format;
    node["eol"] = config.eol;
    node["filename"] = config.filename;
    node["roll_type"] = config.roll_type;
    node["reserve_count"] = config.reserve_count;
    node["roll_size"] = config.roll_size;
    node["rotation_hour"] = config.rotation_hour;
    node["rotation_minute"] = config.rotation_minute;
    node["remove_timeout_file_switch"] = config.remove_timeout_file_switch;
    node["hour_interval"] = config.hour_interval;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::LocalFileSinkConfig& config) {
    if (node["format"]) {
      config.format = node["format"].as<std::string>();
    }
    if (node["eol"]) {
      config.eol = node["eol"].as<bool>();
    }
    if (node["filename"]) {
      config.filename = node["filename"].as<std::string>();
    }
    if (node["roll_type"]) {
      config.roll_type = node["roll_type"].as<std::string>();
    }
    if (node["reserve_count"]) {
      config.reserve_count = node["reserve_count"].as<unsigned int>();
    }
    if (node["roll_size"]) {
      config.roll_size = node["roll_size"].as<unsigned int>();
    }
    if (node["rotation_hour"]) {
      config.rotation_hour = node["rotation_hour"].as<unsigned int>();
    }
    if (node["rotation_minute"]) {
      config.rotation_minute = node["rotation_minute"].as<unsigned int>();
    }
    if (node["remove_timeout_file_switch"]) {
      config.remove_timeout_file_switch = node["remove_timeout_file_switch"].as<bool>();
    }
    if (node["hour_interval"]) {
      config.hour_interval = node["hour_interval"].as<unsigned int>();
    }
    return true;
  }
};

}  // namespace YAML
