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

#pragma once

#include "yaml-cpp/yaml.h"

#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {

/// @brief Fiber Concurrent configuration.
struct FiberLimiterControlConf {
  uint32_t max_fiber_count{60000};  ///< The maximum number of concurrent fibers.

  bool is_report{false};  ///< Whether to report result to the monitoring plugin

  /// @brief Display the value of the configuration field.
  void Display() const;
};
}  // namespace trpc::overload_control

namespace YAML {

template <>
struct convert<trpc::overload_control::FiberLimiterControlConf> {
  static YAML::Node encode(const trpc::overload_control::FiberLimiterControlConf& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::FiberLimiterControlConf& config);
};

}  // namespace YAML

#endif
