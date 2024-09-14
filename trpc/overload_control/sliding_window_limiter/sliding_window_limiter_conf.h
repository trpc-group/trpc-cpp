/*
 *
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2024 THL A29 Limited, a Tencent company.
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

#include "yaml-cpp/yaml.h"

#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {

/// @brief sliding_window concurrent control configuration.
struct SlidingWindowLimiterControlConf {
  // Maximum number of requests per second.
  int64_t limit{1000};
  // Window size
  int32_t window_size{100};
  // Whether to report the judgment result to the monitoring plugin.
  bool is_report{false};

  /// @brief Display the value of the configuration field.
  void Display() const;
};
}  // namespace trpc::overload_control

namespace YAML {

template <>
struct convert<trpc::overload_control::SlidingWindowLimiterControlConf> {
  static YAML::Node encode(const trpc::overload_control::SlidingWindowLimiterControlConf& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::SlidingWindowLimiterControlConf& config);
};

}  // namespace YAML

#endif