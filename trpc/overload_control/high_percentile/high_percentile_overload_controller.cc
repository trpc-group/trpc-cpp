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

#include "trpc/overload_control/high_percentile/high_percentile_overload_controller.h"

#include "trpc/metrics/trpc_metrics.h"
#include "trpc/overload_control/common/report.h"
#include "trpc/overload_control/common/request_priority.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

HighPercentileOverloadController::HighPercentileOverloadController(const Options& options) : options_(options) {
  std::vector<HighAvgStrategy*> strategies;

  // Maximum thread/coroutine scheduling time strategy.
  if (options.expected_max_schedule_delay != std::chrono::steady_clock::duration::zero()) {
    schedule_delay_strategy_ = std::make_unique<HighAvgStrategy>(HighAvgStrategy::Options{
        .name = kExpectedMaxScheduleDelay,
        .expected = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(options.expected_max_schedule_delay).count()),
    });
    strategies.emplace_back(schedule_delay_strategy_.get());
  }

  // Maximum downstream request time scheduling strategy.
  if (options.expected_max_request_latency != std::chrono::steady_clock::duration::zero()) {
    request_latency_strategy_ = std::make_unique<HighAvgStrategy>(HighAvgStrategy::Options{
        .name = kExpectedMaxRequestDelay,
        .expected = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(options.expected_max_request_latency).count()),
    });
    strategies.emplace_back(request_latency_strategy_.get());
  }

  high_percentile_priority_ = std::make_unique<HighPercentilePriorityImpl>(HighPercentilePriorityImpl::Options{
      .service_func = options_.service_func,
      .is_report = options_.is_report,
      .strategies = strategies,
      .min_concurrency_window_size = options_.min_concurrency_window_size,
      .min_max_concurrency = options_.min_max_concurrency,
  });

  auto adapter_options = options.adapter_options;
  // Combine high percentile priority processing objects with priority adapters.
  adapter_options.priority = high_percentile_priority_.get();
  adapter_options.report_name = options_.service_func;
  priority_adapter_ = std::make_unique<PriorityAdapter>(adapter_options);
}

bool HighPercentileOverloadController::OnRequest(const ServerContextPtr& context) {
  PriorityAdapter::Result result = priority_adapter_->OnRequest(GetServerPriority(context));
  bool passed = (result == PriorityAdapter::Result::kOK);
  // Report judgment results.
  if (options_.is_report) {
    OverloadInfo infos;
    infos.attr_name = kOverloadctrlHighPercentile;
    infos.report_name = options_.service_func;
    infos.tags[kOverloadctrlPass] = (result == PriorityAdapter::Result::kOK ? 1 : 0);
    infos.tags[kOverloadctrlLimitedByLowerPriority] = (result == PriorityAdapter::Result::kLimitedByLower ? 1 : 0);
    infos.tags[kOverloadctrlLimited] = (result == PriorityAdapter::Result::kLimitedByOverload ? 1 : 0);
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
  return passed;
}

void HighPercentileOverloadController::OnResponse(const ServerContextPtr& context) {
  // Ignore requests that fail for any reason.
  if (!context->GetStatus().OK()) {
    priority_adapter_->OnError();
    return;
  }

  // Update monitoring metric strategy, which will be used in the EMA algorithm for high percentile.
  auto now_ms = trpc::time::GetMilliSeconds();
  if (schedule_delay_strategy_) {
    auto schedule_delay_ms = context->GetBeginTimestamp() - context->GetRecvTimestamp();
    if (options_.is_report) {
      Report::GetInstance()->ReportStrategySampleInfo(StrategySampleInfo{
          .report_name = options_.service_func,
          .strategy_name = schedule_delay_strategy_->Name(),
          .value = schedule_delay_ms,
      });
    }
    schedule_delay_strategy_->Sample(schedule_delay_ms);
  }
  if (request_latency_strategy_) {
    auto request_latency_ms = now_ms - context->GetRecvTimestamp();
    if (options_.is_report) {
      Report::GetInstance()->ReportStrategySampleInfo(StrategySampleInfo{
          .report_name = options_.service_func,
          .strategy_name = request_latency_strategy_->Name(),
          .value = request_latency_ms,
      });
    }
    request_latency_strategy_->Sample(request_latency_ms);
  }

  // Update request cost data.
  auto cost_ms = now_ms - context->GetRecvTimestamp();

  if (options_.is_report) {
    Report::GetInstance()->ReportStrategySampleInfo(StrategySampleInfo{
        .report_name = options_.service_func,
        .strategy_name = "",
        .value = cost_ms,
    });
  }

  priority_adapter_->OnSuccess(std::chrono::milliseconds(cost_ms));
}

}  // namespace trpc::overload_control

#endif
