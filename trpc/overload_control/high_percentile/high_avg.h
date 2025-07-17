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

#include <array>
#include <mutex>

#include "trpc/overload_control/high_percentile/max_in_every_n.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::overload_control {

/// @brief Calculate the high percentile mean value of recent data to detect the calculation of indicators.
class HighAvg {
 public:
  /// The statistical period for each `MaxInEveryN` maximum value calculator.
  static constexpr int kN = 30;
  /// The number of `MaxInEveryN` maximum value calculators.
  static constexpr int kK = 3;

 public:
  HighAvg();

  /// @brief Perform a step using the recently obtained sampled values and return the current
  ///        calculated high percentile mean value.
  /// @return The average value of type double.
  double AdvanceAndGet();

  /// @brief Add a sampled value.
  /// @param value The sampled value.
  void Sample(double value);

 private:
  // Spin lock used for sampling.
  Spinlock lock_;

  // Current high percentile mean value.
  double value_;

  // The sum of the recent sampled values.
  double total_;

  // The number of times the recent sampling has been performed.
  int times_;

  // Period maximum value calculator.
  std::array<MaxInEveryN, kK> max_containers_;
};

}  // namespace trpc::overload_control

#endif
