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

#include <memory>
#include <string>
#include <unordered_map>

#include "trpc/metrics/metrics.h"

namespace trpc {

/// @brief Plugin factory class for metrics
/// @note `Register` interface is not thread-safe and needs to be called when the program starts
class MetricsFactory {
 public:
  /// @brief Get the instance of MetricsFactory.
  /// @return Return the pointer of MetricsFactory
  static MetricsFactory* GetInstance() {
    static MetricsFactory instance;
    return &instance;
  }

  /// @brief Register a metrics plugin instance to the factory
  /// @param metrics the pointer of metrics plugin instance
  /// @return true: success, false: failed
  bool Register(const MetricsPtr& metrics);

  /// @brief Get the metrics plugin instance according to the plugin name
  /// @param name plugin name
  /// @return Returns the pointer of metrics plugin instance on success, nullptr otherwise
  MetricsPtr Get(const std::string& name) const;

  /// @brief Get all metrics plugins
  const std::unordered_map<std::string, MetricsPtr>& GetAllPlugins();

  /// @brief Clear metrics plugins
  void Clear();

 private:
  MetricsFactory() = default;
  MetricsFactory(const MetricsFactory&) = delete;
  MetricsFactory& operator=(const MetricsFactory&) = delete;

 private:
  std::unordered_map<std::string, MetricsPtr> metrics_plugins_;
};

}  // namespace trpc
