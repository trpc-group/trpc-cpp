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

#include "trpc/overload_control/high_percentile/high_avg.h"

#include <mutex>

#include "trpc/util/check.h"

namespace trpc::overload_control {

HighAvg::HighAvg()
    : value_(0),
      total_(0),
      times_(0),
      max_containers_({
          MaxInEveryN(kN),
          MaxInEveryN(kN),
          MaxInEveryN(kN),
      }) {
  // Perform a certain number of samples for different calculators, so that their cycles are evenly staggered.
  for (int i = 0; i < kK; ++i) {
    auto& container = max_containers_[i];
    for (int j = 0; j < kN / kK * i; ++j) {
      container.Sample(0);
    }
  }
}

double HighAvg::AdvanceAndGet() {
  // If the lock acquisition fails, return the result of the previous attempt.
  std::unique_lock lk(lock_, std::try_to_lock);
  if (!lk.owns_lock()) {
    return value_;
  }

  if (times_ == 0) {
    return value_;
  }

  // Fixed coefficient EMA algorithm, where the coefficient is derived from the formula St = 0.9*St + avg(Vmax).
  value_ = value_ * 0.9 + total_ / times_ * 0.1;
  total_ = 0.0;
  times_ = 0;

  return value_;
}

void HighAvg::Sample(double value) {
  // If the lock acquisition fails, abandon the sampling.
  std::unique_lock lk(lock_, std::try_to_lock);
  if (!lk.owns_lock()) {
    return;
  }

  for (auto& container : max_containers_) {
    auto [expired, max_value] = container.Sample(value);
    if (expired) {
      total_ += max_value;
      ++times_;

      // It must be a finite value.
      TRPC_CHECK(std::isfinite(total_), "HighAvg::total_ overflowed");
    }
  }
}

}  // namespace trpc::overload_control

#endif
