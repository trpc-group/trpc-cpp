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
#include <vector>

#include "trpc/overload_control/common/priority.h"
#include "trpc/overload_control/common/window.h"
#include "trpc/overload_control/high_percentile/high_avg_ema.h"
#include "trpc/overload_control/high_percentile/high_avg_strategy.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::overload_control {

/// @brief Priority request processing based on high percentile statistical indicators.
///        The maximum concurrency is dynamically updated mainly through the EAM algorithm.
/// @note Just for server side
class HighPercentilePriorityImpl final : public Priority {
 public:
  /// @brief Options
  struct Options {
    /// The combination of service name and function name is used for reporting.
    std::string service_func;
    /// Whether to report
    bool is_report;
    /// Metric strategies
    std::vector<HighAvgStrategy*> strategies;
    /// The minimum number of update concurrency window requests.
    int min_concurrency_window_size;
    /// The guaranteed minimum concurrency.
    int min_max_concurrency;
  };

 public:
  explicit HighPercentilePriorityImpl(const Options& options);

  /// @brief Process high-priority requests by algorithm the result of which determine whether this request is allowed.
  /// @return true: success；false: failed
  bool MustOnRequest() override;

  /// @brief Process requests by algorithm the result of which determine whether this request is allowed.
  /// @return true: success；false: failed
  bool OnRequest() override;

  /// @brief Update algorithm data by recording the cost time of the request after it is processed successfully
  /// @param cost Time cost
  void OnSuccess(std::chrono::steady_clock::duration cost) override;

  /// @brief Update algorithm data when an error occurs during request processing.
  void OnError() override;

 private:
  // Using the specified maximum concurrency, check if a request can pass.
  bool Acquire(int max_concurrency);

  // Execute when the window expires, re-estimate the maximum concurrency, and reset the window.
  void OnWindowExpires();

  // Based on the current statistical strategy state, perform offset correction on the cost,
  // amplify it during light load, and reduce it during overload.
  double ShiftCost(double cost, std::vector<double>& measureds);

 private:
  Options options_;

  // When updating the concurrency using a mutex lock, you need to first try to acquire the lock using try_lock,
  // and then proceed if successful.
  // Only used for try_lock to acquire the lock without blocking.
  // When the service has a high QPS (queries per second) and fails to acquire the lock, the metric statistics
  // switch to sampling statistics, but the overall trend remains the same.
  Spinlock lock_;

  // Current concurrency.
  std::atomic_int cur_concurrency_;
  // The algorithm calculates the maximum allowed concurrency count.
  std::atomic_int max_concurrency_;

  // All statistical monitoring strategies.
  std::vector<HighAvgStrategy*> strategies_;
  /// The current active statistical monitoring strategy.
  std::atomic<HighAvgStrategy*> in_effect_strategy_;

  // The EMA (Exponential Moving Average) algorithm object is used to obtain the average cost of recent service
  // requests.
  HighAvgEMA cost_ms_ema_;
  // The EMA (Exponential Moving Average) algorithm object is used to obtain the average QPS (queries per second) of
  // recent requests.
  HighAvgEMA qps_ema_;

  // Periodic counting window.
  Window window_;

  // The number of successful requests within the window period.
  std::atomic_int succeeded_;
  // The total accumulated time for successful requests with the prefix "succeeded_".
  std::atomic_int costs_ms_;
};

}  // namespace trpc::overload_control

#endif
