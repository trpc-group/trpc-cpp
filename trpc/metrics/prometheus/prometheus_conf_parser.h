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

#include "trpc/metrics/prometheus/prometheus_conf.h"

namespace YAML {

template <>
struct convert<trpc::PrometheusConfig> {
  static YAML::Node encode(const trpc::PrometheusConfig& conf) {
    YAML::Node node;
    node["histogram_module_cfg"] = conf.histogram_module_cfg;
    node["const_labels"] = conf.const_labels;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::PrometheusConfig& conf) {
    if (node["histogram_module_cfg"]) {
      conf.histogram_module_cfg = node["histogram_module_cfg"].as<std::vector<double>>();
    }

    if (node["const_labels"]) {
      conf.const_labels = node["const_labels"].as<std::map<std::string, std::string>>();
    }
    return true;
  }
};

}  // namespace YAML
