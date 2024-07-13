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

#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_conf.h"

#include "trpc/log/trpc_log.h"

namespace trpc::overload_control {

void TokenBucketLimiterControlConf::Display() const {
  TRPC_FMT_DEBUG("----------TokenBucketLimiterControlConf---------------");

  TRPC_FMT_DEBUG("capacity: {}", capacity);
  TRPC_FMT_DEBUG("rate: {}", rate);
  TRPC_FMT_DEBUG("is_report: {}", is_report);
}
}  // namespace trpc::overload_control

namespace YAML {

YAML::Node convert<trpc::overload_control::TokenBucketLimiterControlConf>::encode(
    const trpc::overload_control::TokenBucketLimiterControlConf& config) {
  YAML::Node node;

  node["capacity"] = config.capacity;
  node["rate"] = config.rate;
  node["is_report"] = config.is_report;

  return node;
}

bool convert<trpc::overload_control::ConcurrencyLimiterControlConf>::decode(
    const YAML::Node& node, trpc::overload_control::ConcurrencyLimiterControlConf& config) {
  if (node["max_concurrency"]) {
    config.max_concurrency = node["max_concurrency"].as<uint32_t>();
  }
  if (node["rate"]) {
    config.rate = node["rate"].as<uint32_t>();
  }
  if (node["is_report"]) {
    config.is_report = node["is_report"].as<bool>();
  }

  return true;
}

}  // namespace YAML

#endif