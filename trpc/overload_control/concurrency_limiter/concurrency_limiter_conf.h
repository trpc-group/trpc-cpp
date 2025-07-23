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

#pragma once

#include "yaml-cpp/yaml.h"

#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {

/// @brief Concurrent flow control configuration.
struct ConcurrencyLimiterControlConf {
  uint32_t max_concurrency{60000};  ///< Maximum concurrency.

  bool is_report{false};  ///< Whether to report the judgment result to the monitoring plugin.

  /// @brief Display the value of the configuration field.
  void Display() const;
};
}  // namespace trpc::overload_control

namespace YAML {

template <>
struct convert<trpc::overload_control::ConcurrencyLimiterControlConf> {
  static YAML::Node encode(const trpc::overload_control::ConcurrencyLimiterControlConf& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::ConcurrencyLimiterControlConf& config);
};

}  // namespace YAML

#endif
