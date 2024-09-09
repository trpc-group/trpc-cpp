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

    for (const auto& [name, serivce] : config.services) {
      YAML::Node service_node;
      service_node["name"] = name;

      for (const auto& [address, weight] : serivce) {
        YAML::Node target_node;
        target_node["address"] = address;
        target_node["weight"] = weight;
        service_node["target"].push_back(target_node);
      }

      node["service"].push_back(service_node);
    }

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::SWRoundrobinLoadBalanceConfig& config) {
    for (const auto& service_node : node["service"]) {
      std::unordered_map<std::string, uint32_t> service_config;
      std::string service_name;
      if (service_node["name"]) {
        service_name = service_node["name"].as<std::string>();
      }

      if (service_node["target"]) {
        for (const auto& target_node : service_node["target"]) {
          std::string address = target_node["address"].as<std::string>();
          uint32_t weight = target_node["weight"].as<uint32_t>();
          service_config[address] = weight;
        }
      }

      config.services[service_name] = service_config;
    }

    return true;
  }
};

}  // namespace YAML
