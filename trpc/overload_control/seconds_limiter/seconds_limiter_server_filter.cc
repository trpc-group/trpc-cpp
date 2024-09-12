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

#include "trpc/overload_control/seconds_limiter/seconds_limiter_server_filter.h"

#include "trpc/filter/filter_manager.h"
#include "trpc/log/trpc_log.h"
#include "trpc/overload_control/flow_control/flow_controller_conf.h"
#include "trpc/overload_control/seconds_limiter/seconds_overload_controller.h"

namespace trpc::overload_control {

int SecondsLimiterServerFilter::Init() {
  std::vector<FlowControlLimiterConf> seconds_limiter_confs;
  LoadFlowControlLimiterConf(seconds_limiter_confs);
  seconds_overload_controller_ = std::make_shared<SecondsOverloadController>();
  seconds_overload_controller_->Init(seconds_limiter_confs);
  return 0;
}

std::vector<FilterPoint> SecondsLimiterServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // This tracking point is not being used, but tracking points must be paired, so it is added here.
      FilterPoint::SERVER_POST_SCHED_RECV_MSG,
  };
}

void SecondsLimiterServerFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
  switch (point) {
    case FilterPoint::SERVER_PRE_SCHED_RECV_MSG: {
      OnRequest(status, context);
      break;
    }
    default: {
      break;
    }
  }
}

void SecondsLimiterServerFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is already a dirty request, it will not be processed further to ensure that the first error code is
    // not overwritten.
    return;
  }
  if (seconds_overload_controller_->BeforeSchedule(context)) {
    status = FilterStatus::REJECT;
  }
  return;
}

}  // namespace trpc::overload_control

#endif
