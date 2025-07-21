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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/fiber_limiter/fiber_limiter_conf.h"

#include "trpc/log/trpc_log.h"

namespace trpc::overload_control {

void FiberLimiterControlConf::Display() const {
  TRPC_FMT_DEBUG("----------FiberLimiterControlConf---------------");

  TRPC_FMT_DEBUG("max_fiber_count: {}", max_fiber_count);
  TRPC_FMT_DEBUG("is_report: {}", is_report);
}
}  // namespace trpc::overload_control

namespace YAML {

YAML::Node convert<trpc::overload_control::FiberLimiterControlConf>::encode(
    const trpc::overload_control::FiberLimiterControlConf& config) {
  YAML::Node node;

  node["max_fiber_count"] = config.max_fiber_count;
  node["is_report"] = config.is_report;

  return node;
}

bool convert<trpc::overload_control::FiberLimiterControlConf>::decode(
    const YAML::Node& node, trpc::overload_control::FiberLimiterControlConf& config) {
  if (node["max_fiber_count"]) {
    config.max_fiber_count = node["max_fiber_count"].as<uint32_t>();
  }
  if (node["is_report"]) {
    config.is_report = node["is_report"].as<bool>();
  }

  return true;
}

}  // namespace YAML

#endif
