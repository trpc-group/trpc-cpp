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

#include "trpc/overload_control/throttler/throttler_conf.h"
#include "trpc/overload_control/common/priority_conf_parse.h"

namespace YAML {

YAML::Node convert<trpc::overload_control::ThrottlerConf>::encode(const trpc::overload_control::ThrottlerConf& config) {
  YAML::Node node;

  node["ratio_for_accepts"] = config.ratio_for_accepts;
  node["requests_padding"] = config.requests_padding;
  node["max_throttle_probability"] = config.max_throttle_probability;
  node["ema_factor"] = config.ema_factor;
  node["ema_interval_ms"] = config.ema_interval_ms;
  node["ema_reset_interval_ms"] = config.ema_reset_interval_ms;

  // Encode priority config
  trpc::overload_control::Encode<trpc::overload_control::ThrottlerConf>(node, config);

  return node;
}

bool convert<trpc::overload_control::ThrottlerConf>::decode(const YAML::Node& node,
                                                            trpc::overload_control::ThrottlerConf& config) {
  trpc::overload_control::DecodeField(node, "ratio_for_accepts", config.ratio_for_accepts);
  trpc::overload_control::DecodeField(node, "requests_padding", config.requests_padding);
  trpc::overload_control::DecodeField(node, "max_throttle_probability", config.max_throttle_probability);
  trpc::overload_control::DecodeField(node, "ema_factor", config.ema_factor);
  trpc::overload_control::DecodeField(node, "ema_interval_ms", config.ema_interval_ms);
  trpc::overload_control::DecodeField(node, "ema_reset_interval_ms", config.ema_reset_interval_ms);

  // Decode priority config
  trpc::overload_control::Decode<trpc::overload_control::ThrottlerConf>(node, config);

  return true;
}

}  // namespace YAML

#endif
