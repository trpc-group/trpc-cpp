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

#include "trpc/overload_control/high_percentile/high_percentile_server_filter.h"

#include "trpc/common/config/trpc_config.h"

namespace trpc::overload_control {

int HighPercentileServerFilter::Init() {
  bool ok = TrpcConfig::GetInstance()->GetPluginConfig<HighPercentileConf>(kOverloadCtrConfField, kHighPercentileName,
                                                                           config_);
  if (!ok) {
    TRPC_LOG_DEBUG("HighPercentileServerFilter read config failed, will use a default config");
  }

  return 0;
}

std::vector<FilterPoint> HighPercentileServerFilter::GetFilterPoint() {
  return {
      // The tracking points must appear in pairs. If one of the tracking points is used, the corresponding other
      // tracking point must also be present.
      // SERVER_PRE_SCHED_RECV_MSG is the stage of pre-scheduling and receiving messages on the server side.
      // Placing a tracking point at this stage ensures that overload protection processing is performed as early as
      // possible.
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      FilterPoint::SERVER_POST_SCHED_RECV_MSG,

      FilterPoint::SERVER_PRE_RPC_INVOKE,
      // This tracking point is used for post-processing after an RPC call to ensure alignment with trpc-go.
      FilterPoint::SERVER_POST_RPC_INVOKE,

      // The following tracking points are only used as a fallback.
      FilterPoint::SERVER_POST_RECV_MSG,
      FilterPoint::SERVER_PRE_SEND_MSG,
  };
}

void HighPercentileServerFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
  if (context->GetCallType() == kOnewayCall) {
    // The SERVER_POST_RPC_INVOKE tracking point will not be executed in one-way scenarios,
    // so it is not suitable for this overload protection.
    TRPC_FMT_DEBUG("Oneway mode, will not run continue");
    return;
  }
  switch (point) {
    case FilterPoint::SERVER_PRE_SCHED_RECV_MSG: {
      OnRequest(status, context);
      break;
    }
    case FilterPoint::SERVER_POST_RPC_INVOKE: {
      OnResponse(status, context);
      break;
    }
    default: {
      if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
        // For other tracking points, in case of errors, OnResponse is executed as a fallback.
        OnResponse(status, context);
      }
      break;
    }
  }
}

void HighPercentileServerFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is a dirty request, processing will not continue afterwards to ensure that the first error code is not
    // overwritten.
    return;
  }
  // Ensure that the incoming request is valid and that both the callee and the function exist. Use the format
  // "func_name" as the key for mapping.
  const HighPercentileOverloadControllerPtr& controller = GetController(context->GetFuncName());
  bool ret = controller->OnRequest(context);
  if (ret) {
    // If the strategy is successful, set the controller for OnResponse
    context->SetFilterData(GetFilterID(), controller);
  } else if (!config_.dry_run) {
    // If the dry run is not enabled, interception is required.
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by high_percentile overload control"));
    status = FilterStatus::REJECT;
  }
}

void HighPercentileServerFilter::OnResponse(FilterStatus& status, const ServerContextPtr& context) {
  HighPercentileOverloadControllerPtr* controller =
      context->GetFilterData<HighPercentileOverloadControllerPtr>(GetFilterID());
  if (!controller || !(*controller)) {
    // If no settings are configured, it means that the request processing has failed.
    return;
  }
  (*controller)->OnResponse(context);
  // Setting it to empty is used to avoid multiple executions.
  *controller = nullptr;
}

HighPercentileOverloadControllerPtr HighPercentileServerFilter::CreateController(const std::string& service_func) {
  return MakeRefCounted<HighPercentileOverloadController>(HighPercentileOverloadController::Options{
      .service_func = service_func,
      .is_report = config_.is_report,
      .expected_max_schedule_delay = std::chrono::milliseconds(config_.max_schedule_delay),
      .expected_max_request_latency = std::chrono::milliseconds(config_.max_request_latency),
      .min_concurrency_window_size = config_.min_concurrency_window_size,
      .min_max_concurrency = config_.min_max_concurrency,
      .adapter_options =
          PriorityAdapter::Options{
              .report_name = service_func,
              .is_report = config_.is_report,
              .max_priority = config_.max_priority,
              .lower_step = config_.lower_step,
              .upper_step = config_.upper_step,
              .fuzzy_ratio = config_.fuzzy_ratio,
              .max_update_interval = std::chrono::milliseconds(config_.max_update_interval),
              .max_update_size = config_.max_update_size,
              .histogram_num = config_.histograms,
              .priority = nullptr,  // It will be created internally.
          },
  });
}

HighPercentileOverloadControllerPtr& HighPercentileServerFilter::GetController(const std::string& service_func) {
  {
    // Read lock
    std::shared_lock lk(lock_);
    auto it = controllers_.find(service_func);
    if (it != controllers_.end()) {
      return it->second;
    }
  }
  // Write lock
  std::unique_lock lk(lock_);
  // It is possible that multiple threads are unable to acquire the read lock during the process, but one of the threads
  // acquires the write lock first. And performs a write operation, other threads will then acquire it at this point.
  auto it = controllers_.find(service_func);
  if (it != controllers_.end()) {
    return it->second;
  }
  // If not found, create a new one. Differentiate the algorithm granularity based on the "func" and align it with the
  // trpc-go.
  controllers_[service_func] = CreateController(service_func);

  return controllers_[service_func];
}

}  // namespace trpc::overload_control

#endif
