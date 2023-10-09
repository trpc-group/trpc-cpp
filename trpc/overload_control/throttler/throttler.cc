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

#include "trpc/overload_control/throttler/throttler.h"

#include <iostream>
#include <mutex>

#include "trpc/util/random.h"

namespace trpc::overload_control {

Throttler::Throttler(const Options& options)
    : options_(options), accepts_ema_(options.ema_options), throttles_ema_(options.ema_options) {}

bool Throttler::OnRequest() {
  // Obtain the recent average of two types of requests: successful requests and requests that failed due to rate
  // limiting, in order to calculate the pass rate.
  // If the lock is acquired successfully, use `Sum()` to obtain the latest average.
  // If the lock acquisition fails, use  `CurrentTotal()` to obtain the average result of the previous `Sum()`
  // calculation, which has minimal impact under high QPS (queries per second).
  double accepts = accepts_ema_.CurrentTotal();
  double throttles = throttles_ema_.CurrentTotal();
  if (auto lk = std::unique_lock(lock_, std::try_to_lock); lk.owns_lock()) {
    auto now = std::chrono::steady_clock::now();
    // Obtain the maximum number of accepts and throttles in the past cycle by the EMA (Exponential Moving Average)
    // algorithm .
    accepts = accepts_ema_.Sum(now);
    throttles = throttles_ema_.Sum(now);
  }

  double requests = accepts + throttles;

  // Rate limiting algorithm formula: (requests - K * accepts) / (requests + requests_padding)
  double throttle_probability =
      (requests - accepts * options_.ratio_for_accepts) / (requests + options_.requests_padding);

  // Limit the maximum rate of rate limiting.
  throttle_probability = std::min(throttle_probability, options_.max_throttle_probability);

  return ::trpc::Random<double>(0.0, 1.0) > throttle_probability;
}

void Throttler::OnResponse(bool throttled) {
  // Only update EMA when the lock is acquired.
  //
  // The algorithm result is only related to the ratio of successful to failed requests:
  // - When the QPS is low, the probability of failing to acquire the lock is low.
  // - When the QPS is high, the ratio of successful to failed requests tends to stabilize.
  std::unique_lock<Spinlock> lk(lock_, std::try_to_lock);
  if (!lk.owns_lock()) {
    return;
  }

  auto now = std::chrono::steady_clock::now();
  (!throttled) ? accepts_ema_.Add(now, 1) : throttles_ema_.Add(now, 1);
}

}  // namespace trpc::overload_control

#endif
