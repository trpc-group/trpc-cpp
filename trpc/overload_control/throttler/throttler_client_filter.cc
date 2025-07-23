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

#include "trpc/overload_control/throttler/throttler_client_filter.h"

#include "fmt/format.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/common/plugin.h"
#include "trpc/log/trpc_log.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"

namespace trpc::overload_control {

int ThrottlerClientFilter::Init() {
  bool ok = TrpcConfig::GetInstance()->GetPluginConfig<ThrottlerConf>(kOverloadCtrConfField, kThrottlerName, config_);
  if (!ok) {
    TRPC_LOG_DEBUG("ThrottlerClientFilter read config failed, will use a default config");
  }
  return 0;
}

std::vector<FilterPoint> ThrottlerClientFilter::GetFilterPoint() {
  return {
      FilterPoint::CLIENT_PRE_RPC_INVOKE,
      FilterPoint::CLIENT_POST_RPC_INVOKE,
      FilterPoint::CLIENT_PRE_SEND_MSG,
      FilterPoint::CLIENT_POST_RECV_MSG,
  };
}

void ThrottlerClientFilter::operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) {
  // One-way mode also performs tracking, so there is no need to distinguish it from the server-side like one-way mode.
  switch (point) {
    // `OnRequest` relies on the naming plugin for addressing and
    // is only guaranteed to be available after `CLIENT_PRE_SEND_MSG`.
    case FilterPoint::CLIENT_PRE_SEND_MSG: {
      OnRequest(status, context);
      break;
    }
    // `OnResponse` relies on message decoding (completed by ClientCodec::FillResponse) and
    //  is only available after `CLIENT_POST_RPC_INVOKE`.
    case FilterPoint::CLIENT_POST_RPC_INVOKE: {
      OnResponse(status, context);
      break;
    }
    default: {
      break;
    }
  }
}

void ThrottlerClientFilter::OnRequest(FilterStatus& status, const ClientContextPtr& context) {
  // Dirty request, not processed.
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    return;
  }

  auto controller = GetOrCreateOverloadControl(context);
  if (!controller) {
    // In general flow control scenarios, there will be a large number of requests. To avoid excessive logging, printing
    // is done once per second.
    TRPC_FMT_WARN_EVERY_SECOND("cant not find control from ip:{}, port:{}, callee name {}", context->GetIp(),
                               context->GetPort(), context->GetCalleeName());
    return;
  }
  bool passed = controller->OnRequest(context);
  // If the strategy is passed, set the filtered data for easy use in `OnResponse`.
  if (passed) {
    context->SetFilterData(GetFilterID(), controller);
  } else if (!config_.dry_run) {
    // If the strategy fails and `dry_run` is not enabled, intercept it.
    context->SetStatus(Status(TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR, 0, "rejected by overload control"));
    status = FilterStatus::REJECT;
  }
}

void ThrottlerClientFilter::OnResponse(FilterStatus& status, const ClientContextPtr& context) {
  ThrottlerOverloadControllerPtr* controller = context->GetFilterData<ThrottlerOverloadControllerPtr>(GetFilterID());
  if (!controller || !(*controller)) {
    return;
  }
  (*controller)->OnResponse(context);
  *controller = nullptr;
}

ThrottlerOverloadControllerPtr ThrottlerClientFilter::GetOrCreateOverloadControl(const ClientContextPtr& context) {
  if (context->GetIp().empty() || context->GetPort() < 0) {
    return nullptr;
  }

  // For some services, the existence of the called method is not guaranteed. In this case,
  // it is more reasonable to compare using IP and port.
  std::string ip_port = fmt::format("{}:{}", context->GetIp(), context->GetPort());

  {
    std::shared_lock lk(mutex_);
    auto it = controllers_.find(ip_port);
    if (it != controllers_.end()) {
      return it->second;
    }
  }

  std::unique_lock lk(mutex_);
  auto it = controllers_.find(ip_port);
  if (it != controllers_.end()) {
    return it->second;
  }

  // If it is still not found, create it.
  ThrottlerOverloadControllerPtr controller =
      MakeRefCounted<ThrottlerOverloadController>(ThrottlerOverloadController::Options{
          .ip_port = ip_port,
          .is_report = config_.is_report,
          .priority_adapter_options =
              PriorityAdapter::Options{
                  .report_name = ip_port,
                  .is_report = config_.is_report,
                  .max_priority = config_.max_priority,
                  .lower_step = config_.lower_step,
                  .upper_step = config_.upper_step,
                  .fuzzy_ratio = config_.fuzzy_ratio,
                  .max_update_interval = std::chrono::milliseconds(config_.max_update_interval),
                  .max_update_size = config_.max_update_size,
                  .histogram_num = config_.histograms,
                  .priority = nullptr,  // will assigned inner
              },
          .throttler_options =
              Throttler::Options{
                  .ratio_for_accepts = config_.ratio_for_accepts,
                  .requests_padding = config_.requests_padding,
                  .max_throttle_probability = config_.max_throttle_probability,
                  .ema_options =
                      ThrottlerEma::Options{
                          .factor = config_.ema_factor,
                          .interval = std::chrono::milliseconds(config_.ema_interval_ms),
                          .reset_interval = std::chrono::milliseconds(config_.ema_reset_interval_ms),
                      },
              },
      });

  controllers_[ip_port] = controller;
  // Regularly remove those that are used less frequently.
  SubmitClearTaskThreadUnsafe(ip_port);

  return controller;
}

void ThrottlerClientFilter::SubmitClearTaskThreadUnsafe(const std::string& ip_port) {
  // Remove old task (if exists)
  if (auto it = task_ids_.find(ip_port); it != task_ids_.end()) {
    PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(it->second);
    task_ids_.erase(it);
  }
  // Submit task
  std::string task_name = "ClientOverloadControlClearTask@" + ip_port;
  auto task = [this, ip_port]() { ClearIfExpired(ip_port); };
  // The timer processes expired tasks at a cycle of 30 seconds.
  auto task_id = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(std::move(task), 30 * 1000,
                                                                                  std::move(task_name));
  task_ids_.insert_or_assign(ip_port, task_id);
}

void ThrottlerClientFilter::ClearIfExpired(const std::string& ip_port) {
  {
    std::shared_lock lk(mutex_);
    if (!CheckIfExpiredThreadUnsafe(ip_port)) {
      return;
    }
  }

  std::unique_lock lk(mutex_);
  if (!CheckIfExpiredThreadUnsafe(ip_port)) {
    return;
  }

  if (controllers_.erase(ip_port) == 0) {
    TRPC_FMT_ERROR("client overload control instance not found, key=\"{}\"", ip_port);
  }

  auto it = task_ids_.find(ip_port);
  if (it == task_ids_.end()) {
    TRPC_FMT_ERROR("client overload control task id not found, key=\"{}\"", ip_port);
    return;
  }
  uint64_t task_id = it->second;
  task_ids_.erase(it);

  // Remove task
  PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id);
}

bool ThrottlerClientFilter::CheckIfExpiredThreadUnsafe(const std::string& ip_port) {
  auto it = controllers_.find(ip_port);
  if (it == controllers_.end()) {
    TRPC_FMT_ERROR("client overload control instance not found, key=\"{}\"", ip_port);
    return true;  // Return true to notify the outer layer to continue trying to stop the task and delete the task_id.
  }
  return it->second->IsExpired();
}

}  // namespace trpc::overload_control

#endif
