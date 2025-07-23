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

#include <utility>

namespace trpc::overload_control {

/// @brief Continuously receive new values and periodically (every N values received) return the maximum value within
///        the period.
///        The initial value is 0, and only non-negative numbers are supported for statistics.
class MaxInEveryN {
 public:
  /// @brief Construct a MaxInEveryN and use n as the period.
  explicit MaxInEveryN(int n);

  /// @brief Receive a new data and return:
  /// - whether the maximum sampling times N has been reached after this sampling
  /// - the maximum value within the current sampling period.
  ///
  /// @note If the maximum sampling times N is reached after this sampling, clear the state and enter the next sampling
  ///       period.
  std::pair<bool, double> Sample(double value);

 private:
  // Maximum sampling period.
  const int period_;

  // Sampling times in the current period.
  int times_;

  // Maximum value within the current sampling period.
  double max_;
};

}  // namespace trpc::overload_control

#endif
