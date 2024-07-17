/*
*
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2023 THL A29 Limited, a Tencent company.
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

/// @brief Token_bucket concurrent control configuration.
struct TokenBucketLimiterControlConf {
  uint32_t capacity{1000};		///< Maximum token bucket count capacity.
  uint32_t initial_token{0};	///< Initial token count.
  uint32_t rate{10};			///< Token production rate(count/second).
  bool is_report{false};		///< Whether to report the judgment result to the monitoring plugin.

  /// @brief Display the value of the configuration field.
  void Display() const;
};
}  // namespace trpc::overload_control

namespace YAML {

template <>
struct convert<trpc::overload_control::TokenBucketLimiterControlConf> {
  static YAML::Node encode(const trpc::overload_control::TokenBucketLimiterControlConf& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::TokenBucketLimiterControlConf& config);
};

}  // namespace YAML

#endif
