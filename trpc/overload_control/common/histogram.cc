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

#include "trpc/overload_control/common/histogram.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "trpc/util/check.h"

namespace trpc::overload_control {

/// @brief The initial value of the counter.
/// @note Using 1 instead of 0 to improve the stability of the algorithm.
static constexpr int32_t kInitialCounterValue = 1;

Histogram::Histogram(const Options& options)
    : options_(options),
      priority_counters_(options_.priorities, kInitialCounterValue),
      total_(options_.priorities * kInitialCounterValue) {}

int32_t Histogram::Total() const { return total_; }

void Histogram::Reset() {
  std::fill(priority_counters_.begin(), priority_counters_.end(), kInitialCounterValue);
  total_ = priority_counters_.size() * kInitialCounterValue;
}

void Histogram::Sample(int32_t priority) {
  // Priority is not less than 0.
  TRPC_CHECK_GE(priority, 0);
  // Priority must be less than the size of the priority array.
  TRPC_CHECK_LT(priority, options_.priorities);
  // The sum of priorities cannot be greater than the maximum value to avoid overflow.
  TRPC_CHECK_NE(total_, std::numeric_limits<int32_t>::max());

  ++priority_counters_[priority];
  ++total_;
}

double Histogram::ShiftFuzzyPriority(double fuzzy_priority, double shift) const {
  if (!shift) {
    // If the conversion coefficient is 0, return directly.
    return fuzzy_priority;
  }

  int32_t priority = static_cast<int32_t>(std::floor(fuzzy_priority));
  // Extract the decimal part of the priority.
  double fuzzy = fuzzy_priority - std::floor(fuzzy_priority);

  if (priority >= options_.priorities) {
    // avoid overflow
    priority = options_.priorities - 1;
    fuzzy = 1.0;
  }

  // Calculate the result of multiplying the coefficient by the value, and round it off using rounding.
  int32_t delta = static_cast<int32_t>(std::round(static_cast<double>(total_) * std::abs(shift)));

  if (delta == 0) {  // Delta should not be 0, otherwise the priority will not be converted.
    delta = 1;
  }

  if (shift > 0) {
    delta += static_cast<int32_t>(std::round(fuzzy * priority_counters_[priority]));
    for (;;) {
      int32_t priority_counter = priority_counters_[priority];

      if (delta <= priority_counter) {
        return static_cast<double>(priority) + static_cast<double>(delta) / static_cast<double>(priority_counter);
      } else if (priority + 1 >= options_.priorities) {
        return static_cast<double>(options_.priorities);
      }

      delta -= priority_counter;
      ++priority;
    }
  } else {
    delta += static_cast<int32_t>(std::round((1 - fuzzy) * static_cast<double>(priority_counters_[priority])));
    for (;;) {
      int32_t priority_counter = priority_counters_[priority];

      if (delta <= priority_counter) {
        return static_cast<double>(priority) +
               static_cast<double>(priority_counter - delta) / static_cast<double>(priority_counter);
      } else if (priority - 1 < 0) {
        return 0.0;
      }

      delta -= priority_counter;
      --priority;
    }
  }

  TRPC_UNREACHABLE();
}

}  // namespace trpc::overload_control

#endif
