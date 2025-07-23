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

#include "trpc/metrics/prometheus/prometheus_conf.h"

namespace YAML {

template <>
struct convert<trpc::PrometheusConfig> {
  static YAML::Node encode(const trpc::PrometheusConfig& conf) {
    YAML::Node node;
    node["histogram_module_cfg"] = conf.histogram_module_cfg;
    node["const_labels"] = conf.const_labels;
    node["auth_cfg"] = conf.auth_cfg;
    node["push_mode"]["enable"] = conf.push_mode.enable;
    if (conf.push_mode.enable) {
      node["push_mode"]["gateway_host"] = conf.push_mode.gateway_host;
      node["push_mode"]["gateway_port"] = conf.push_mode.gateway_port;
      node["push_mode"]["job_name"] = conf.push_mode.job_name;
      node["push_mode"]["interval_ms"] = conf.push_mode.interval_ms;
    }
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::PrometheusConfig& conf) {
    if (node["histogram_module_cfg"]) {
      conf.histogram_module_cfg = node["histogram_module_cfg"].as<std::vector<double>>();
    }

    if (node["const_labels"]) {
      conf.const_labels = node["const_labels"].as<std::map<std::string, std::string>>();
    }

    if (node["auth_cfg"]) {
      conf.auth_cfg = node["auth_cfg"].as<std::map<std::string, std::string>>();
    }
    if (node["push_mode"]) {
      const auto& push_mode = node["push_mode"];
      conf.push_mode.enable = push_mode["enable"].as<bool>(false);
      if (conf.push_mode.enable) {
        conf.push_mode.gateway_host = push_mode["gateway_host"].as<std::string>();
        conf.push_mode.gateway_port = push_mode["gateway_port"].as<std::string>();
        conf.push_mode.job_name = push_mode["job_name"].as<std::string>();
        conf.push_mode.interval_ms = push_mode["interval_ms"].as<int>(10000);
      }
    }
    return true;
  }
};

}  // namespace YAML
