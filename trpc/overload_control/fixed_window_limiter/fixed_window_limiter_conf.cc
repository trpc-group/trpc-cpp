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

#include "trpc/log/trpc_log.h"

#include "trpc/overload_control/fixed_window_limiter/fixed_window_limiter_conf.h"

namespace trpc::overload_control {

void FixedWindowLimiterControlConf::Display() const {
  TRPC_FMT_DEBUG("----------FixedWindowLimiterControlConf---------------");

  TRPC_FMT_DEBUG("limit: {}", limit);
  TRPC_FMT_DEBUG("is_report: {}", is_report);
}
}  // namespace trpc::overload_control

namespace YAML {

YAML::Node convert<trpc::overload_control::FixedWindowLimiterControlConf>::encode(
    const trpc::overload_control::FixedWindowLimiterControlConf& config) {
  YAML::Node node;

  node["limit"] = config.limit;
  node["is_report"] = config.is_report;

  return node;
}

bool convert<trpc::overload_control::FixedWindowLimiterControlConf>::decode(
    const YAML::Node& node, trpc::overload_control::FixedWindowLimiterControlConf& config) {
  if (node["limit"]) {
    config.limit = node["limit"].as<int64_t>();
  }
  if (node["is_report"]) {
    config.is_report = node["is_report"].as<bool>();
  }

  return true;
}

}  // namespace YAML

#endif
