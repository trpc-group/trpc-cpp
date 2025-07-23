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

#include "trpc/overload_control/common/priority_adapter.h"

#include <mutex>

#include "trpc/overload_control/common/report.h"
#include "trpc/util/check.h"
#include "trpc/util/random.h"

namespace trpc::overload_control {

/// @brief The minimum value for fuzzy processing of priority.
constexpr double kMinFuzzyPriority = 0.0;
/// @brief The maximum value for fuzzy processing of priority.
constexpr double kMaxFuzzyPriority = 1.0;

/// @brief The maximum ratio of "may_ok / may".
constexpr double kMaxOkMayRatio = 0.5;

/// @brief The coefficient for updating priority when there is too little sample data.
constexpr int32_t kMaxPriorityFactor = 3;

/// @brief The coefficient for the expiration interval of the window.
constexpr uint32_t kWindowIntervalFactor = 2;

/// @brief The coefficient for updating the maximum passing data of the window.
constexpr uint32_t kMaxUpdateFactor = 2;

PriorityAdapter::PriorityAdapter(const Options& options)
    : options_(options),
      window_(options.max_update_interval, options.max_update_size),
      histograms_(options_.histogram_num, Histogram(Histogram::Options{.priorities = options_.max_priority + 1})),
      histogram_index_(0),
      priority_(options.priority) {
  TRPC_CHECK(priority_ != nullptr);
  threshold_.lower_ = 0.0;
  threshold_.upper_ = static_cast<double>(options_.max_priority) + 1;

  requests_.must_ = 0;
  requests_.may_ = 0;
  requests_.ok_ = 0;
  requests_.no_ = 0;
}

PriorityAdapter::Result PriorityAdapter::OnRequest(int32_t priority) {
  priority = std::max(0, std::min(options_.max_priority, priority));
  Result result = ApplyRequest(priority);

  // Perform priority sampling and trigger window update only when the lock is successfully acquired.
  std::unique_lock lk(lock_, std::try_to_lock);
  if (!lk.owns_lock()) {
    return result;
  }

  // Perform one sampling of priority distribution.
  Sample(priority);

  // Trigger the window even if the request is not successful. This check must be performed here because in dry_run
  // mode, failures may not be executed, so it cannot be judged in the OnError method class.
  if (result != Result::kOK && window_.Touch()) {
    OnWindowExpires();
  }

  return result;
}

void PriorityAdapter::OnSuccess(std::chrono::steady_clock::duration cost) {
  priority_->OnSuccess(cost);

  std::unique_lock lk(lock_, std::try_to_lock);
  if (!lk.owns_lock()) {
    return;
  }

  if (window_.Touch()) {
    OnWindowExpires();
  }
}

void PriorityAdapter::OnError() { priority_->OnError(); }

void PriorityAdapter::OnWindowExpires() {
  auto& histogram = histograms_[histogram_index_];

  // If the total number of samples in the histogram is not enough, abandon the update of the priority threshold and
  // increase the next window cycle.
  if (histogram.Total() < options_.max_priority * kMaxPriorityFactor) {
    auto current_window_interval = window_.GetInterval();
    // Update window data.
    window_.Reset(current_window_interval * kWindowIntervalFactor, options_.max_update_size * kMaxUpdateFactor);
    return;
  }

  double lower = threshold_.lower_;
  double upper = threshold_.upper_;

  // Record the current request-related information.
  PriorityAdapter::Requests requests{
      .must_ = requests_.must_.load(),
      .may_ = requests_.may_.load(),
      .ok_ = requests_.ok_.load(),
      .no_ = requests_.no_.load(),
  };

  // Adjust the size of "upper" based on the relationship between "ok/must" and "fuzzy_ratio".
  if (requests.must_ == 0 ||
      (static_cast<double>(requests.ok_) / static_cast<double>(requests.must_) > options_.fuzzy_ratio)) {
    upper = histogram.ShiftFuzzyPriority(upper, -options_.upper_step);
  } else {
    upper = histogram.ShiftFuzzyPriority(upper, options_.upper_step);
  }

  // Adjust the size of "lower" based on the relationship between "ok/may" and "0.5".
  if (requests.may_ == 0 || (static_cast<double>(requests.ok_) / static_cast<double>(requests.may_) > kMaxOkMayRatio)) {
    lower = histogram.ShiftFuzzyPriority(lower, -options_.lower_step);
  } else {
    lower = histogram.ShiftFuzzyPriority(lower, options_.lower_step);
  }

  lower = std::min(lower, upper);
  threshold_.lower_ = lower;
  threshold_.upper_ = upper;

  if (options_.is_report) {
    OverloadInfo priority_infos;
    priority_infos.attr_name = kOverloadctrlPriority;
    priority_infos.report_name = options_.report_name;
    priority_infos.tags["lower"] = lower;
    priority_infos.tags["upper"] = upper;
    priority_infos.tags["must"] = requests_.must_;
    priority_infos.tags["may"] = requests_.may_;
    priority_infos.tags["ok"] = requests_.ok_;
    priority_infos.tags["no"] = requests_.no_;

    Report::GetInstance()->ReportOverloadInfo(priority_infos);
  }

  // Clean up data
  requests_.must_ = 0;
  requests_.may_ = 0;
  requests_.ok_ = 0;
  requests_.no_ = 0;

  histogram.Reset();
  histogram_index_ = (histogram_index_ + 1) % (histograms_.size());

  window_.Reset(options_.max_update_interval, options_.max_update_size);
}

PriorityAdapter::Result PriorityAdapter::ApplyRequest(int32_t priority) {
  // Perform fuzzy processing on integer priority to obtain a floating-point priority. The specific implementation is
  // to add a random fuzzy value in the range of [0, 1).
  double fuzzy_priority = priority + ::trpc::Random<double>(kMinFuzzyPriority, kMaxFuzzyPriority);

  // Intercept if the priority is lower than "lower_".
  if (fuzzy_priority < threshold_.lower_) {
    ++requests_.no_;
    // The request has been intercepted due to its low priority.
    return Result::kLimitedByLower;
  }

  // If the priority is higher than "upper_", apply for MustOnRequest.
  if (fuzzy_priority > threshold_.upper_) {
    bool approved = priority_->MustOnRequest();
    if (!approved) {
      ++requests_.must_;  // punishment
    }
    ++requests_.must_;
    return approved ? Result::kOK : Result::kLimitedByOverload;
  }

  // The priority is in [lower_, upper_]
  ++requests_.may_;
  bool approved = priority_->OnRequest();
  if (approved) {
    ++requests_.ok_;
  }
  return approved ? Result::kOK : Result::kLimitedByOverload;
}

void PriorityAdapter::Sample(int32_t priority) {
  for (auto& histogram : histograms_) {
    histogram.Sample(priority);
  }
}

}  // namespace trpc::overload_control

#endif
