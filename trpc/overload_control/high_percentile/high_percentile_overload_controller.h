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
#include <memory>
#include <optional>

#include "trpc/overload_control/common/priority_adapter.h"
#include "trpc/overload_control/high_percentile/high_avg_strategy.h"
#include "trpc/overload_control/high_percentile/high_percentile_priority_impl.h"
#include "trpc/server/server_context.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::overload_control {

/// @brief Overload protection controller based on monitoring high percentile values.
///        Manage related dependent objects and provide calling entry points.
class HighPercentileOverloadController : public RefCounted<HighPercentileOverloadController> {
 public:
  /// @brief Options
  struct Options {
    /// Combine service name and method name for reporting.
    std::string service_func;
    /// Whether the judgment result is reported to the monitoring plugin
    bool is_report;
    /// The expected maximum delay for thread/coroutine scheduling, set to 0 to disable this policy.
    std::chrono::steady_clock::duration expected_max_schedule_delay;
    /// The expected maximum delay for requests, set to 0 to disable this policy.
    std::chrono::steady_clock::duration expected_max_request_latency;
    /// The minimum number of concurrent window update requests.
    int min_concurrency_window_size;
    /// The guaranteed minimum concurrency.
    int min_max_concurrency;
    /// PriorityAdapter options
    PriorityAdapter::Options adapter_options;
  };

 public:
  explicit HighPercentileOverloadController(const Options& options);

  /// @brief Process requests by algorithm the result of which determine whether this request is allowed.
  /// @param context Server context
  /// @return true: success; false: failed
  bool OnRequest(const ServerContextPtr& context);

  /// @brief Process the response of current request to update algorithm data for the next request processing.
  /// @param context Server context
  void OnResponse(const ServerContextPtr& context);

 private:
  Options options_;

  // Instance of overload protection based on high percentiles.
  std::unique_ptr<HighPercentilePriorityImpl> high_percentile_priority_;

  // Priority-based overload protection algorithm.
  std::unique_ptr<PriorityAdapter> priority_adapter_;

  // Thread/coroutine scheduling delay strategy.
  std::unique_ptr<HighAvgStrategy> schedule_delay_strategy_;

  // Request delay (i.e. handle delay) strategy.
  std::unique_ptr<HighAvgStrategy> request_latency_strategy_;
};

using HighPercentileOverloadControllerPtr = RefPtr<HighPercentileOverloadController>;
}  // namespace trpc::overload_control

#endif
