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

#include <map>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

namespace trpc {

/// @brief Prometheus metrics plugin configuration
struct PrometheusConfig {
  /// Statistical interval for the distribution of execution time for inter-module calls, and its unit is milliseconds.
  std::vector<double> histogram_module_cfg{1, 10, 100, 1000};

  /// The default label attached to each RPC metrics data
  std::map<std::string, std::string> const_labels;

  std::map<std::string, std::string> auth_cfg;

  struct PushMode {
    bool enable = false;
    std::string gateway_host;
    std::string gateway_port;
    std::string job_name;
    int interval_ms = 10000;
  } push_mode;

  void Display() const;
};

}  // namespace trpc
