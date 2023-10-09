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

#include <string>
#include <unordered_map>

#include "trpc/telemetry/telemetry.h"

namespace trpc {

/// @brief Plugin factory class for telemetry
/// @note `Register` interface is not thread-safe and needs to be called when the program starts
class TelemetryFactory {
 public:
  /// @brief Gets the instance of TelemetryFactory.
  /// @return the pointer of TelemetryFactory
  static TelemetryFactory* GetInstance() {
    static TelemetryFactory instance;
    return &instance;
  }

  /// @brief Registers a telemetry plugin instance to the factory
  /// @param telemetry the pointer of telemetry plugin instance
  /// @return true: success, false: failed
  bool Register(const TelemetryPtr& telemetry);

  /// @brief Gets the telemetry plugin instance according to the plugin name
  /// @param name plugin name
  /// @return the pointer of telemetry plugin instance on success, nullptr otherwise
  TelemetryPtr Get(const std::string& name);

  /// @brief Gets all telemetry plugins
  /// @return the plugin map which key is the name and value is the plugin instance
  const std::unordered_map<std::string, TelemetryPtr>& GetAllPlugins();

  /// @brief Clear telemetry plugins
  void Clear();

 private:
  TelemetryFactory() = default;
  TelemetryFactory(const TelemetryFactory&) = delete;
  TelemetryFactory& operator=(const TelemetryFactory&) = delete;

 private:
  std::unordered_map<std::string, TelemetryPtr> telemetry_plugins_;
};

}  // namespace trpc
