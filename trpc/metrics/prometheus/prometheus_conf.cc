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

#include "trpc/metrics/prometheus/prometheus_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void PrometheusConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("histogram_module_cfg:");
  for (size_t i = 0; i < histogram_module_cfg.size(); i++) {
    TRPC_LOG_DEBUG(i << " " << histogram_module_cfg[i]);
  }

  TRPC_LOG_DEBUG("const_labels:");
  for (auto label : const_labels) {
    TRPC_LOG_DEBUG(label.first << ":" << label.second);
  }

  TRPC_LOG_DEBUG("--------------------------------");
}

}  // namespace trpc

namespace YAML {

YAML::Node convert<trpc::PrometheusConfig>::encode(const trpc::PrometheusConfig& config) {
  YAML::Node node;
  node["histogram_module_cfg"] = config.histogram_module_cfg;
  node["const_labels"] = config.const_labels;
  node["auth_cfg"] = config.auth_cfg;
  return node;
}

bool convert<trpc::PrometheusConfig>::decode(const YAML::Node& node, trpc::PrometheusConfig& config) {
  if (node["histogram_module_cfg"]) {
    config.histogram_module_cfg = node["histogram_module_cfg"].as<std::vector<double>>();
  }
  if (node["const_labels"]) {
    config.const_labels = node["const_labels"].as<std::map<std::string, std::string>>();
  }
  if (node["auth_cfg"]) {
    config.auth_cfg = node["auth_cfg"].as<std::map<std::string, std::string>>();
  }
  return true;
}

}  // namespace YAML
