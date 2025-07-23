/*
 *
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2023 Tencent.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

namespace trpc::overload_control {

/// @brief The configuration of interface-level flow control
struct FuncLimiterConfig {
  /// Interface: such as: SayHello
  std::string name;

  /// Flow control limiter, standard format: name (maximum limit per second),such as: default(100000)
  std::string limiter;

  /// @brief Algorithm sampling window for interface-level
  int32_t window_size{0};

  /// @brief Print conf information
  void Display() const;
};

/// @brief The configuration of flow control
struct FlowControlLimiterConf {
  /// @brief Service name
  std::string service_name;

  /// @brief Service-level flow control limiter
  /// @example standard format: name (maximum limit per second), such as: default(100000)
  std::string service_limiter;

  /// @brief Algorithm sampling window for service-level
  int32_t window_size{0};

  /// Set of interface-level flow control
  std::vector<FuncLimiterConfig> func_limiters;

  /// @brief Whether to report monitoring data.
  bool is_report{false};

  /// @brief Display the value of the configuration field.
  void Display() const;
};

void LoadFlowControlLimiterConf(std::vector<FlowControlLimiterConf>& flow_control_confs);

}  // namespace trpc::overload_control

namespace YAML {

template <>
struct convert<trpc::overload_control::FuncLimiterConfig> {
  static YAML::Node encode(const trpc::overload_control::FuncLimiterConfig& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::FuncLimiterConfig& config);
};

template <>
struct convert<trpc::overload_control::FlowControlLimiterConf> {
  static YAML::Node encode(const trpc::overload_control::FlowControlLimiterConf& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::FlowControlLimiterConf& config);
};

}  // namespace YAML

#endif
