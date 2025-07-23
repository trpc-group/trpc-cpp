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

#include "trpc/common/plugin.h"
#include "trpc/log/logging.h"
#include "trpc/metrics/metrics.h"
#include "trpc/tracing/tracing.h"

namespace trpc {

/// @brief The abstract interface definition for telemetry plugins.
/// @note The telemetry plugin is positioned as a plugin that integrates tracing, metrics, and log capabilities.
///       For tracing, it need to inherit from the Tracing plugin and collect tracing data through filter mechanism.
///       For metrics, it need to inherit from the Metrics plugin, collect metrics data through filter mechanism,
///       and implementing the interface for custom metrics data reporting.
///       For logging, it need to inherit from the LoggingBase plugin and register to DefaultLog.
class Telemetry : public Plugin {
 public:
  /// @brief Gets the plugin type
  /// @return plugin type
  PluginType Type() const override { return PluginType::kTelemetry; }

  /// @brief Gets the instance of Tracing plugin which implementing tracing capabilities
  virtual TracingPtr GetTracing() = 0;

  /// @brief Gets the instance of Metrics plugin which implementing metrics capabilities
  virtual MetricsPtr GetMetrics() = 0;

  /// @brief Gets the instance of Logging plugin which implementing log capabilities
  virtual LoggingPtr GetLog() = 0;
};

using TelemetryPtr = RefPtr<Telemetry>;

}  // namespace trpc
