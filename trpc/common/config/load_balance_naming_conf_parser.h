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
#include "trpc/common/config/load_balance_naming_conf.h"
#include "yaml-cpp/yaml.h"

namespace YAML {

template <>
struct convert<trpc::naming::SWRoundrobinLoadBalanceConfig> {
  static YAML::Node encode(const trpc::naming::SWRoundrobinLoadBalanceConfig& config) {
    YAML::Node node;
    // Iterate over each service in the config
    for (const auto& [service_name, weights] : config.services_weight) {
      YAML::Node service_node;

      service_node["service"] = service_name;

      service_node["weights"] = weights;

      node.push_back(service_node);
    }
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::SWRoundrobinLoadBalanceConfig& config) {
    // Iterate over each service node in the YAML
    for (const auto& service_node : node) {
      std::string service_name;
      if (service_node["service"]) {
        service_name = service_node["service"].as<std::string>();
      }

      std::vector<uint32_t> weights;
      if (service_node["weights"]) {
        weights = service_node["weights"].as<std::vector<uint32_t>>();
      }
      config.services_weight[service_name] = weights;
    }

    return true;
  }
};

}  // namespace YAML
