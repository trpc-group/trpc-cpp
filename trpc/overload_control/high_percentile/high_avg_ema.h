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

namespace trpc::overload_control {

/// @brief A simple algorithm implementation of a variant of EMA (Exponential Moving Average) suitable for
///        HighAvg algorithm to calculate the average value over a window of time, used for calculating the maximum and
///        minimum values of QPS and average latency.
///
///    S(t) = X(t) * α + S(t - 1) * (1 - α), if X(t) > S(t - 1)
///    S(t) = X(t) * β + S(t - 1) * (1 - β), if X(t) <= S(t - 1)
///
///    Where 0 < α << 1, 0 < β << 1, usually α != β.
///
/// For calculating min(cost_ms), α << β can be used, for example, α = 0.1, β = 0.01 (fast increase and slow decrease).
/// For calculating max(qps), a >> β can be used, for example, α = 0.01, β = 0.1 (slow increase and fast decrease).
///
/// @note Unthread-safe
class HighAvgEMA {
 public:
  /// @brief EMA algorithm factors
  struct Options {
    double inc_factor;  ///< Increase the factor, range [0.0, 1.0].
    double dec_factor;  ///< Decrease the factor, range [0.0, 1.0].
  };

 public:
  explicit HighAvgEMA(const Options&);

  /// @brief Update a new value and return the newly calculated average.
  /// @param sample_val Sample Value
  /// @return The newly calculated average as a double type.
  double UpdateAndGet(double sample_val);

  /// @brief Get the current average value.
  /// @return The average value as a double type.
  double Get();

 private:
  // Increase the factor
  double inc_factor;
  // Decrease the factor
  double dec_factor;
  // Current average value
  double val_;
};

}  // namespace trpc::overload_control

#endif
