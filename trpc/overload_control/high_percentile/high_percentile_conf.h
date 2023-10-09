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

/// @brief HighPercentile algorithm configuration.
struct HighPercentileConf {
  /// Expected maximum scheduling delay (in ms). If exceeded, it is considered overloaded. A value of 0 means this
  /// load metric is not enabled. It is recommended to set it to 15 or 20.
  int max_schedule_delay{15};

  /// Expected maximum downstream request delay (in ms). If exceeded, it is considered overloaded.
  /// A value of 0 means this load metric is not enabled.
  int max_request_latency{0};

  /// Minimum number of concurrent window update requests.
  /// If the number of requests in a single window period is less than this value,
  /// it is considered too small to estimate the maximum concurrency.
  int min_concurrency_window_size{200};

  /// Guaranteed minimum concurrency.
  /// If the algorithm estimates that the system concurrency is lower than this value, this value will be used directly
  /// as the upper limit of concurrency.
  int min_max_concurrency{10};

  /// Request priority upper limit. If a request carries a higher priority, it will be truncated.
  int max_priority{255};

  /// Perform metric statistics and overload algorithms without intercepting requests, for experimental observation.
  bool dry_run{false};

  /// Whether to report overload protection monitoring metrics.
  bool is_report{true};

  /// The speed of automatic threshold adjustment for lower.
  double lower_step{0.02};

  /// The speed of automatic threshold adjustment for upper.
  double upper_step{0.01};

  /// Priority fuzzy range, which refers to the desired value of N_OK / N_MUST to be maintained.
  double fuzzy_ratio{0.1};

  /// Maximum window duration (in milliseconds).
  int max_update_interval{100};

  /// Maximum window sampling count.
  int max_update_size{512};

  /// Number of priority distribution statistical histograms, and the reasonable configuration range should be [2, 8].
  int histograms{3};
};
}  // namespace trpc::overload_control

namespace YAML {

template <>
struct convert<trpc::overload_control::HighPercentileConf> {
  static YAML::Node encode(const trpc::overload_control::HighPercentileConf& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::HighPercentileConf& config);
};

}  // namespace YAML
#endif
