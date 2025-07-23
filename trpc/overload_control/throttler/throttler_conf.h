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

/// @brief Throttler algorithm configuration.
struct ThrottlerConf {
  /// Incremental acceptance rate, usually a value slightly greater than 1, indicating that "slight" overload is
  /// allowed.
  double ratio_for_accepts{1.3};

  /// A constant padding used when calculating the flow rate based on success rate, which reduces the chance of
  /// misjudgment at low QPS.
  double requests_padding{8};

  /// Maximum flow rate limit.
  double max_throttle_probability{0.7};

  /// throttler ema variation factor.
  double ema_factor{0.8};

  /// throttler ema Window period.
  int ema_interval_ms{100};

  /// throttler ema full expiration time of the window.
  int ema_reset_interval_ms{30 * 1000};

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
struct convert<trpc::overload_control::ThrottlerConf> {
  static YAML::Node encode(const trpc::overload_control::ThrottlerConf& config);

  static bool decode(const YAML::Node& node, trpc::overload_control::ThrottlerConf& config);
};

}  // namespace YAML

#endif
