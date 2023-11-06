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
#include <memory>
#include <string>
#include <vector>

#include "trpc/overload_control/common/histogram.h"
#include "trpc/overload_control/common/priority.h"
#include "trpc/overload_control/common/window.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::overload_control {

/// @brief Overload Control priority adapter.
/// Combine and encapsulate algorithms that implement the Priority interface, and support protection interception by
//  judging whether to execute based on the priority passed in.
class PriorityAdapter {
 public:
  /// @brief Configuration options required for class initialization.
  struct Options {
    std::string report_name;                                  ///< Name for reporting.
    bool is_report;                                           ///< whether to report the priority threshold status
    int32_t max_priority;                                     ///< Max priority
    double lower_step;                                        ///< Step size of low priority threshold change.
    double upper_step;                                        ///< Step size of upper priority threshold change.
    double fuzzy_ratio;                                       ///< Desired ratio of "may_ok_requests / must_requests".
    std::chrono::steady_clock::duration max_update_interval;  ///< Time interval for updating the window period.
    int32_t max_update_size;                                  ///< Number of requests for updating the window period.
    int32_t histogram_num;                                    ///< The number of histogram
    Priority* priority{nullptr};                              ///< Implementation of overload protection algorithm.
  };
  /// @brief Result of request approval.
  enum class Result {
    kOK,                 ///< Pass
    kLimitedByLower,     ///< Request intercepted due to low priority.
    kLimitedByOverload,  ///< Overload interception.
  };

 public:
  explicit PriorityAdapter(const Options& options);

  /// @brief Process requests by algorithm the result of which determine whether this request is allowed.
  /// @param priority Request priority.
  /// @return  Result type which is representation of execution result.
  Result OnRequest(int32_t priority);

  /// @brief  Update algorithm data by recording the cost time of the request after it is processed successfully
  /// @param cost Time spent on a successful request.
  void OnSuccess(std::chrono::steady_clock::duration cost);

  /// @brief Update algorithm data when an error occurs during request processing.
  void OnError();

 private:
  // Request application based on priority.
  Result ApplyRequest(int32_t priority);

  // Sample a priority.
  void Sample(int32_t priority);

  // Window expiration handling.
  void OnWindowExpires();

 private:
  Options options_;

  // When operating on ‘threshold_` and `histograms_`, a mutex lock is required. In the current implementation,
  // only try_lock is used without blocking.
  Spinlock lock_;

  // Priority threshold.
  // The update threshold operation will only be executed when lock_ is held.
  struct Threshold {
    // Low priority threshold. If the request priority is lower than this value, it will be intercepted directly.
    std::atomic<double> lower_;
    // High priority threshold. If the request priority is higher than this value, MustOnRequest will be used.
    std::atomic<double> upper_;
  } threshold_;

  // Count of different types of requests within a period.
  // The reset operation can only be executed when lock_ is held.
  // The operation of adding 1 may be executed concurrently.
  struct Requests {
    std::atomic_int must_;  ///< Number of requests with priority higher than `upper_`.
    std::atomic_int may_;   ///< Number of requests with priority in the range [lower_, upper_].
    std::atomic_int ok_;    ///< Number of requests with priority in the range [lower_, upper_] and passed the judgment.
    std::atomic_int no_;    ///< Number of requests with priority lower than `lower_`.
  } requests_;

  // Sample window
  Window window_;

  // Histogram of priority
  std::vector<Histogram> histograms_;
  // The index in histogram
  size_t histogram_index_;
  // Priority request class for overload protection processing.
  Priority* priority_;
};

}  // namespace trpc::overload_control

#endif
