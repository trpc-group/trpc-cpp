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

#include <string>

#include "trpc/overload_control/high_percentile/high_avg.h"

namespace trpc::overload_control {

/// @brief HighAvgStrategy is a wrapper for HighAvg.
///        Express a metric monitoring strategy that can obtain the strategy name, expected metric,
///        actual average metric, and add new sampling values.
class HighAvgStrategy {
 public:
  /// @brief Options
  struct Options {
    std::string name;  ///< Strategy name.
    double expected;   ///< Expected metric value under this strategy.
  };

 public:
  explicit HighAvgStrategy(const Options& options);

  /// @brief Get strategy name
  /// @return Strategy name of type sting
  const std::string& Name();

  /// @brief Get the expected metric value of the strategy.
  /// @return Expected value of type double.
  double GetExpected() const;

  /// @brief Get the metric value monitored by the strategy.
  /// @return The measured value of type double
  double GetMeasured();

  /// @brief Collect a new sample value.
  /// @param value The sampled value.
  void Sample(double value);

 private:
  Options options_;

  // Solve the high percentile mean value object.
  HighAvg high_avg_;
};

}  // namespace trpc::overload_control

#endif
