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

#include <chrono>

namespace trpc::overload_control {

/// @brief The EMA (Exponential Moving Average) algorithm used for the Throttler, with a timeout discard mechanism for
///        existing data.
/// @note  Except for the CurrentTotal method, the rest of the methods do not have thread safety guarantees.
class ThrottlerEma {
 public:
  struct Options {
    // Variation factor.
    double factor = 0.8;
    // Window interval.
    std::chrono::steady_clock::duration interval = std::chrono::milliseconds(100);
    // Interval for complete expiration of data.
    std::chrono::steady_clock::duration reset_interval = std::chrono::seconds(30);
  };

 public:
  explicit ThrottlerEma(const Options& options);

  /// @brief Add new data, Unthread-safe
  /// @param time_point Time point
  /// @param value New data
  void Add(std::chrono::steady_clock::time_point time_point, double value);

  /// @brief Get the current total data, thread-safe, but the data may not be the latest.
  /// @return The total sum after adding the value, as a double type.
  double CurrentTotal() const;

  /// @brief Get the current statistical data, Unthread-safe.
  /// @param time_point Time point
  /// @return Total count at the current moment, as a double type.
  double Sum(std::chrono::steady_clock::time_point time_point);

 private:
  // New stock data that reduces the influence of old data.
  void Advance(std::chrono::steady_clock::time_point time_point);

 private:
  Options options_;

  // Last time
  std::chrono::steady_clock::time_point last_;
  // Total
  double total_;
};

}  // namespace trpc::overload_control

#endif
