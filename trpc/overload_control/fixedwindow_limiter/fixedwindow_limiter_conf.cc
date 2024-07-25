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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "fixedwindow_limiter_conf.h"

#include "trpc/log/trpc_log.h"

namespace trpc::overload_control {

void FixedTimeWindowControlConf::Display() const {
  TRPC_FMT_DEBUG("----------FixedWindowLimiterControlConf---------------");

  TRPC_FMT_DEBUG("limit: {}", limit);
  TRPC_FMT_DEBUG("window_size: {}", window_size);
  TRPC_FMT_DEBUG("is_report: {}", is_report);
}

}  // namespace trpc::overload_control

namespace YAML {

YAML::Node convert<trpc::overload_control::FixedTimeWindowControlConf>::encode(
    const trpc::overload_control::FixedTimeWindowControlConf& config) {
  YAML::Node node;

  node["limit"] = config.limit;
  node["window_size"] = config.window_size;
  node["is_report"] = config.is_report;

  return node;
}

bool convert<trpc::overload_control::FixedTimeWindowControlConf>::decode(
    const YAML::Node& node, trpc::overload_control::FixedTimeWindowControlConf& config) {
  if (node["limit"]) {
    config.limit = node["limit"].as<uint32_t>();
  }
  if (node["window_size"]) {
    config.window_size = node["window_size"].as<uint32_t>();
  }
  if (node["is_report"]) {
    config.is_report = node["is_report"].as<bool>();
  }

  return true;
}

}  // namespace YAML

#endif
