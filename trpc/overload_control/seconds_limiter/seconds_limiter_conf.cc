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

#include "trpc/overload_control/seconds_limiter/seconds_limiter_conf.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/log/trpc_log.h"
#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {

void SecondsFuncLimiterConfig::Display() const {
  TRPC_FMT_DEBUG("------------SecondsFuncLimiterConfig--------------------");

  TRPC_FMT_DEBUG("name: {}", name);
  TRPC_FMT_DEBUG("limiter: {}", limiter);
  TRPC_FMT_DEBUG("func limiter window_size: {}", window_size);

  TRPC_FMT_DEBUG("--------------------------------");
}

void SecondsLimiterConf::Display() const {
  TRPC_FMT_DEBUG("----------SecondsLimiterConf---------------");

  TRPC_FMT_DEBUG("service_name: {}", service_name);
  TRPC_FMT_DEBUG("service_limiter: {}", service_limiter);
  for (auto& conf : func_limiters) {
    conf.Display();
  }
  TRPC_FMT_DEBUG("is_report: {}", is_report);
  TRPC_FMT_DEBUG("service limiter window_size: {}", window_size);
}

void LoadSecondsLimiterConf(std::vector<SecondsLimiterConf>& seconds_limiter_confs) {
  YAML::Node seconds_limiter_nodes;
  SecondsLimiterConf seconds_limiter_conf;
  if (ConfigHelper::GetInstance()->GetConfig({"plugins", kOverloadCtrConfField, kSecondsLimiterName},
                                             seconds_limiter_nodes)) {
    for (const auto& node : seconds_limiter_nodes) {
      auto seconds_limiter_conf = node.as<SecondsLimiterConf>();
      seconds_limiter_confs.emplace_back(std::move(seconds_limiter_conf));
    }
  }
}

}  // namespace trpc::overload_control

namespace YAML {

YAML::Node convert<trpc::overload_control::SecondsFuncLimiterConfig>::encode(
    const trpc::overload_control::SecondsFuncLimiterConfig& func_limiter_config) {
  YAML::Node node;

  node["name"] = func_limiter_config.name;
  node["limiter"] = func_limiter_config.limiter;
  node["window_size"] = func_limiter_config.window_size;

  return node;
}

bool convert<trpc::overload_control::SecondsFuncLimiterConfig>::decode(
    const YAML::Node& node, trpc::overload_control::SecondsFuncLimiterConfig& func_limiter_config) {
  if (node["name"]) {
    func_limiter_config.name = node["name"].as<std::string>();
  }
  if (node["limiter"]) {
    func_limiter_config.limiter = node["limiter"].as<std::string>();
  }

  if (node["window_size"]) {
    func_limiter_config.window_size = node["window_size"].as<int32_t>();
  }

  return true;
}

YAML::Node convert<trpc::overload_control::SecondsLimiterConf>::encode(
    const trpc::overload_control::SecondsLimiterConf& config) {
  YAML::Node node;

  node["service_name"] = config.service_name;
  node["service_limiter"] = config.service_limiter;
  node["window_size"] = config.window_size;
  node["func_limiter"] = config.func_limiters;
  node["is_report"] = config.is_report;

  return node;
}

bool convert<trpc::overload_control::SecondsLimiterConf>::decode(
    const YAML::Node& node, trpc::overload_control::SecondsLimiterConf& config) {
  if (node["service_name"]) {
    config.service_name = node["service_name"].as<std::string>();
  }
  if (node["service_limiter"]) {
    config.service_limiter = node["service_limiter"].as<std::string>();
  }

  if (node["window_size"]) {
    config.window_size = node["window_size"].as<int32_t>();
  }

  if (node["func_limiter"]) {
    for (size_t idx = 0; idx < node["func_limiter"].size(); ++idx) {
      auto item = node["func_limiter"][idx].as<trpc::overload_control::SecondsFuncLimiterConfig>();
      config.func_limiters.push_back(item);
    }
  }

  if (node["is_report"]) {
    config.is_report = node["is_report"].as<bool>();
  }

  return true;
}

}  // namespace YAML

#endif
