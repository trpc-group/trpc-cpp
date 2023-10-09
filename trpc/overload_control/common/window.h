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

#include <atomic>
#include <chrono>

namespace trpc::overload_control {

/// @brief Counting Window.
class Window {
 public:
  explicit Window(std::chrono::steady_clock::duration interval, int32_t max_size);

  /// @brief Set the cycle time option, and reset the timestamp and counter.
  /// @param interval Counting cycle.
  /// @param max_size The maximum count value within the window.
  /// @note Thread-unsafe.
  void Reset(std::chrono::steady_clock::duration interval, int32_t max_size);

  /// @brief Indicate the next new data for the current window cycle.
  /// @note Return whether the window is expired after a touch (including expiration due to timeout or
  ///       exceeding the maximum data count).
  bool Touch();

  /// @brief Get the expiration duration for the current window cycle.
  /// @return Counting cycle of type `std::chrono::steady_clock::duration`.
  inline std::chrono::steady_clock::duration GetInterval() const { return interval_; }

  /// @brief Get the duration between the current time and the last timing.
  /// @return Interval duration of type `std::chrono::steady_clock::duration`.
  std::chrono::steady_clock::duration GetLastInterval() const;

 private:
  // Duration of the current window cycle
  std::chrono::steady_clock::duration interval_;

  // Start time of the current window cycle.
  std::chrono::steady_clock::time_point start_;

  // Expiration time of the current window cycle.
  std::chrono::steady_clock::time_point end_;

  // Maximum number of data receptions for the current window cycle. If exceeded, 
  // the current window is also considered expired.
  int32_t max_size_;

  // Number of times new data is added during the window cycle.
  std::atomic_int size_;
};

}  // namespace trpc::overload_control

#endif
