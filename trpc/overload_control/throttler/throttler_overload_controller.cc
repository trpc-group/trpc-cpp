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

#include "trpc/overload_control/throttler/throttler_overload_controller.h"

#include "trpc/overload_control/common/report.h"
#include "trpc/overload_control/common/request_priority.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

// Expiration time, used to determine whether the controller needs to be deleted.
constexpr uint32_t kExpiredTime = 30;

ThrottlerOverloadController::ThrottlerOverloadController(const Options& options)
    : options_(options), time_point_(std::chrono::steady_clock::now()) {
  throttler_ = std::make_unique<Throttler>(options.throttler_options);
  throttler_priority_impl_ = std::make_unique<ThrottlerPriorityImpl>(ThrottlerPriorityImpl::Options{
      .throttler = throttler_.get(),
  });

  auto adapter_options = options.priority_adapter_options;
  adapter_options.priority = throttler_priority_impl_.get();
  adapter_options.report_name = options.ip_port;
  priority_adapter_ = std::make_unique<PriorityAdapter>(adapter_options);
}

bool ThrottlerOverloadController::OnRequest(const ClientContextPtr& context) {
  time_point_ = std::chrono::steady_clock::now();

  auto result = priority_adapter_->OnRequest(GetClientPriority(context));

  // If the request is intercepted due to low priority (priority < lower), the algorithm will not be called.
  // The algorithm can sacrifice itself and return the token when it is tried to calculate successfully,
  // but the request will still be rejected in the end.
  // If the simulated attempt shows that this "virtual request" is not intercepted, the cost saved by this request can
  // be used to return to the token pool, allowing more high-priority requests to pass by borrowing these tokens during
  // overload.
  // When the "virtual request" passes, the current "virtual request" success is used as a hypothesis to
  // update the throttler state (i.e. throttler_->OnResponse(false)). This mechanism improves algorithm stability and
  // reduces false limits.
  if (result == PriorityAdapter::Result::kLimitedByLower) {
    // The parent class didn't execute this method, so it is added here to perform a similar simulation of a virtual
    // request.
    // Throttler's OnRequest and OnResponse methods need to be called in pairs.
    if (throttler_->OnRequest()) {
      throttler_->OnResponse(false);
      // Tokens are added to be used by high-priority requests in the future.
      throttler_priority_impl_->FillToken();
    } else {
      throttler_->OnResponse(true);
    }
  }

  bool passed = (result == PriorityAdapter::Result::kOK);

  // Report result
  if (options_.is_report) {
    OverloadInfo infos;
    infos.attr_name = kOverloadctrlThrottler;
    infos.report_name = options_.ip_port;
    infos.tags[kOverloadctrlPass] = (result == PriorityAdapter::Result::kOK ? 1 : 0);
    infos.tags[kOverloadctrlLimitedByLowerPriority] = (result == PriorityAdapter::Result::kLimitedByLower ? 1 : 0);
    infos.tags[kOverloadctrlLimited] = (result == PriorityAdapter::Result::kLimitedByOverload ? 1 : 0);
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
  return passed;
}

void ThrottlerOverloadController::OnResponse(const ClientContextPtr& context) {
  time_point_ = std::chrono::steady_clock::now();

  auto& status = context->GetStatus();
  // If the request fails, update according to the failure attribution.
  if (!status.OK()) {
    priority_adapter_->OnError();
    throttler_->OnResponse(IsOverloadException(status));
    return;
  }
  // If the request succeeds, record the cost of this request.
  auto cost_ms = trpc::time::GetMilliSeconds() - 1000 * context->GetSendTimestampUs();
  priority_adapter_->OnSuccess(std::chrono::milliseconds(cost_ms));
  throttler_->OnResponse(false);
}

bool ThrottlerOverloadController::IsOverloadException(const Status& status) {
  switch (status.GetFrameworkRetCode()) {
    case TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR:
    case TrpcRetCode::TRPC_SERVER_LIMITED_ERR:
    case TrpcRetCode::TRPC_CLIENT_LIMITED_ERR:
    case TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR:
      return true;
    default:
      return false;
  }
}

bool ThrottlerOverloadController::IsExpired() {
  std::chrono::steady_clock::time_point time_point = time_point_;
  return std::chrono::steady_clock::now() - time_point > std::chrono::seconds(kExpiredTime);
}

}  // namespace trpc::overload_control

#endif
