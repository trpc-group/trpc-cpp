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

#include "trpc/overload_control/high_percentile/high_percentile_conf.h"
#include "trpc/overload_control/common/priority_conf_parse.h"

namespace YAML {

YAML::Node convert<trpc::overload_control::HighPercentileConf>::encode(
    const trpc::overload_control::HighPercentileConf& config) {
  YAML::Node node;

  node["max_schedule_delay"] = config.max_schedule_delay;
  node["max_request_latency"] = config.max_request_latency;
  node["min_concurrency_window_size"] = config.min_concurrency_window_size;
  node["min_max_concurrency"] = config.min_max_concurrency;

  // Encode priority config
  trpc::overload_control::Encode<trpc::overload_control::HighPercentileConf>(node, config);

  return node;
}

bool convert<trpc::overload_control::HighPercentileConf>::decode(const YAML::Node& node,
                                                                 trpc::overload_control::HighPercentileConf& config) {
  trpc::overload_control::DecodeField(node, "max_schedule_delay", config.max_schedule_delay);
  trpc::overload_control::DecodeField(node, "max_request_latency", config.max_request_latency);
  trpc::overload_control::DecodeField(node, "min_concurrency_window_size", config.min_concurrency_window_size);
  trpc::overload_control::DecodeField(node, "min_max_concurrency", config.min_max_concurrency);

  // decode priority config
  trpc::overload_control::Decode<trpc::overload_control::HighPercentileConf>(node, config);

  return true;
}

}  // namespace YAML
#endif
