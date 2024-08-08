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

#include "trpc/overload_control/high_percentile/high_percentile_priority_impl.h"

#include <algorithm>
#include <mutex>

#include "fmt/chrono.h"
#include "fmt/format.h"

#include "trpc/log/trpc_log.h"
#include "trpc/overload_control/common/report.h"

namespace trpc::overload_control {

/// Minimum window period.
static constexpr auto kMinWindowInterval = std::chrono::milliseconds(1);
/// Maximum window period.
static constexpr auto kMaxWindowInterval = std::chrono::milliseconds(100);

/// In the monitoring strategy, the minimum ratio of [monitored value/expected value].
/// If the ratio is lower than this value, it indicates that the system is under light load according to
/// this monitoring strategy.
static constexpr double kMinStrategyQuotient = 0.5;
/// In the monitoring strategy, the maximum threshold of [monitored value/expected value].
/// If the ratio is higher than this value, it indicates that the system is under heavy load according to
/// this monitoring strategy.
static constexpr double kMaxStrategyQuotient = 0.9;

/// Default window size.
static constexpr int32_t kDefaultWinSize = 200;

HighPercentilePriorityImpl::HighPercentilePriorityImpl(const Options& options)
    : options_(options),
      cur_concurrency_(0),
      max_concurrency_(0),
      strategies_(options.strategies),
      in_effect_strategy_(nullptr),
      cost_ms_ema_(HighAvgEMA::Options{
          .inc_factor = 0.01,
          .dec_factor = 0.1,
      }),
      qps_ema_(HighAvgEMA::Options{
          .inc_factor = 0.1,
          .dec_factor = 0.01,
      }),
      window_(kMinWindowInterval, kDefaultWinSize) {}

bool HighPercentilePriorityImpl::MustOnRequest() {
  // For high-priority must requests, relax the max_concurrency limit to double to achieve higher priority.
  return Acquire(max_concurrency_ * 2);
}

bool HighPercentilePriorityImpl::OnRequest() {
  // For medium-priority may requests, use the normally estimated maximum concurrency as the upper limit.
  return Acquire(max_concurrency_);
}

void HighPercentilePriorityImpl::OnSuccess(std::chrono::steady_clock::duration cost) {
  ++succeeded_;
  costs_ms_ += std::chrono::duration_cast<std::chrono::milliseconds>(cost).count();
  if (cur_concurrency_ > 0) --cur_concurrency_;

  // If the window has not expired yet, do nothing.
  if (!window_.Touch()) {
    return;
  }

  // If the window has expired, only the requests that have successfully acquired the lock are responsible for
  //  calculating and executing updates.
  if (auto lk = std::unique_lock(lock_, std::try_to_lock); lk.owns_lock()) {
    OnWindowExpires();
  }
}

void HighPercentilePriorityImpl::OnError() {
  if (cur_concurrency_ > 0) --cur_concurrency_;
}

bool HighPercentilePriorityImpl::Acquire(int max_concurrency) {
  HighAvgStrategy* in_effect_strategy = in_effect_strategy_;

  // If no policy is triggered, pass directly.
  if (in_effect_strategy == nullptr) {
    ++cur_concurrency_;
    return true;
  }

  // If a policy is triggered, check if the concurrency level is too high.
  if (cur_concurrency_ > max_concurrency) {
    TRPC_FMT_WARN_EVERY_SECOND("request(s) rejected by strategy: {}", in_effect_strategy->Name());
    return false;
  }

  // If the concurrency check passes, increase the current concurrency counter.
  ++cur_concurrency_;
  return true;
}

void HighPercentilePriorityImpl::OnWindowExpires() {
  // If the number of requests within the window period is too low, do not perform updates and consider
  // expanding the window appropriately.
  if (succeeded_ < options_.min_concurrency_window_size) {
    auto new_window_interval = window_.GetInterval() * 2;
    if (new_window_interval > kMaxWindowInterval) {
      new_window_interval = kMaxWindowInterval;
    }

    window_.Reset(new_window_interval, options_.min_concurrency_window_size);

    succeeded_ = 0;
    costs_ms_ = 0;

    return;
  }

  double succeeded = static_cast<double>(succeeded_);
  double costs_ms = static_cast<double>(costs_ms_);

  // During high concurrency, the window duration is extremely short, requiring a relatively high precision duration
  //  (e.g. nanoseconds, microseconds).
  // If milliseconds are used for counting, it is possible to get 0, resulting in an infinite QPS calculation.
  auto nano_seconds = std::chrono::duration_cast<std::chrono::nanoseconds>(window_.GetLastInterval()).count();
  double last_seconds = static_cast<double>(nano_seconds) / 1e9;

  double avg_cost_ms = costs_ms / succeeded;
  double qps = succeeded / last_seconds;

  // Use the EMA algorithm to obtain the maximum and minimum values.
  double min_avg_cost_ms = cost_ms_ema_.UpdateAndGet(avg_cost_ms);
  double max_qps = qps_ema_.UpdateAndGet(qps);

  // Estimate the new maximum concurrency level and record the update.
  std::vector<double> measureds;
  double shifted_cost_ms = ShiftCost(min_avg_cost_ms, measureds);
  max_concurrency_ = std::max(static_cast<int>(shifted_cost_ms / 1000 * max_qps), options_.min_max_concurrency);

  // Report the updated concurrency level.
  if (options_.is_report) {
    OverloadInfo curr_infos;
    curr_infos.attr_name = kOverloadctrlConcurrency;
    curr_infos.report_name = options_.service_func;
    for (uint32_t i = 0; i < strategies_.size(); i++) {
      double expected = strategies_[i]->GetExpected();
      double measured = measureds[i];
      curr_infos.tags[fmt::format("{}/expected", strategies_[i]->Name())] = expected;
      curr_infos.tags[fmt::format("{}/measured", strategies_[i]->Name())] = measured;
      curr_infos.tags[fmt::format("{}/quotient", strategies_[i]->Name())] = measured / expected;
    }

    curr_infos.tags["max_measured"] = *std::max_element(measureds.begin(), measureds.end());
    curr_infos.tags["max_concurrency"] = max_concurrency_;
    curr_infos.tags["cur_concurrency"] = cur_concurrency_;

    Report::GetInstance()->ReportOverloadInfo(curr_infos);
  }
  auto new_window_interval = std::chrono::milliseconds(static_cast<int>(min_avg_cost_ms)) * 2;
  if (new_window_interval < kMinWindowInterval) {
    new_window_interval = kMinWindowInterval;
  }
  if (new_window_interval > kMaxWindowInterval) {
    new_window_interval = kMaxWindowInterval;
  }

  auto new_window_size = (avg_cost_ms / 1000) * qps;
  if (new_window_size < options_.min_concurrency_window_size) {
    new_window_size = options_.min_concurrency_window_size;
  }

  window_.Reset(new_window_interval, new_window_size);

  succeeded_ = 0;
  costs_ms_ = 0;
}

double HighPercentilePriorityImpl::ShiftCost(double cost, std::vector<double>& measureds) {
  double max_quotient = kMinStrategyQuotient;
  HighAvgStrategy* critical_strategy = nullptr;

  // Calculate the max quotient through monitoring items.
  for (HighAvgStrategy* strategy : strategies_) {
    // Perform sampling in the HighPercentileOverloadController.
    double expected = strategy->GetExpected();
    double measured = strategy->GetMeasured();
    double quotient = measured / expected;
    if (options_.is_report) {
      measureds.push_back(measured);
    }
    if (quotient > max_quotient) {
      max_quotient = quotient;
      critical_strategy = strategy;
    }
  }

  if (max_quotient <= kMinStrategyQuotient) {
    // There are currently very few requests, so there is no need for interception.
    in_effect_strategy_ = nullptr;
  } else if (max_quotient > kMaxStrategyQuotient || in_effect_strategy_ != nullptr) {
    // In case of high load, a strategy needs to be adopted.
    in_effect_strategy_ = critical_strategy;
  }

  // When the ratio is less than or equal to 1, use (measured/expected) directly as the ratio for amplification.
  if (max_quotient <= 1) {
    return cost / max_quotient;
  }

  // When the ratio is greater than 1, reduce the ratio.
  // This calculation method is approximately equal to sqrt(measured/expected).
  return cost / (1 + (max_quotient - 1) / 2);
}

}  // namespace trpc::overload_control

#endif
