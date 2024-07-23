//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//
#pragma once

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/loadbalance_naming_conf.h"

namespace YAML {

template <>
struct convert<trpc::naming::LoadBalanceSelectorConfig> {
  static YAML::Node encode(const trpc::naming::LoadBalanceSelectorConfig& config) {
    YAML::Node node;
    node["hash_nodes"] = config.hash_nodes;
    node["hash_args"] = config.hash_args;
    node["hash_func"] = config.hash_func;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::LoadBalanceSelectorConfig& config) {
    if (node["hash_nodes"]) {
      config.hash_nodes = node["hash_nodes"].as<uint32_t>();
    }
    if (node["hash_args"]) {
      config.hash_args = node["hash_args"].as<std::vector<uint32_t>>();
    }
    if (node["hash_func"]) {
      config.hash_func = node["hash_func"].as<std::string>();
    }
    return true;
  }
};
}  // namespace YAML