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

#include <stdint.h>

#include <vector>

namespace trpc::overload_control {

/// @brief Histogram class for sampling the count distribution under various priorities.
class Histogram {
 public:
  /// @brief Initialization options.
  struct Options {
    // Number of priorities. Priorities within the range [0, priorities) will be supported.
    int32_t priorities;
  };

 public:
  explicit Histogram(const Options& options);

  /// @brief Get the total number of samples in the current cycle.
  /// @return Return the sum of all priorities.
  int32_t Total() const;

  /// @brief Reset data, reset all priorities to default values(1).
  void Reset();

  /// @brief Perform priority sampling.
  /// @param priority Priority level to be used.
  void Sample(int32_t priority);

  /// @brief Perform fuzzy processing on priorities based on the existing priority distribution.
  /// @param priority Priority
  /// @param shift Conversion coefficient.
  /// @return Value after fuzzy processing.
  double ShiftFuzzyPriority(double priority, double shift) const;

 private:
  Options options_;

  // Priority statistical distribution, where the index represents the priority and the corresponding value represents 
  // the number of occurrences of that priority within a cycle.
  std::vector<int32_t> priority_counters_;

  // Number of priority samples taken within a statistical cycle.
  int32_t total_;
};

}  // namespace trpc::overload_control

#endif
