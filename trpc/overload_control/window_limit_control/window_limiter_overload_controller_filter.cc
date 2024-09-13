//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/window_limit_control/window_limiter_overload_controller_filter.h"

#include "trpc/util/likely.h"
#include "trpc/overload_control/flow_control/flow_controller_conf.h"

namespace trpc::overload_control {

int WindowLimiterOverloadControlFilter::Init() { return controller_->Init(); }

WindowLimiterOverloadControlFilter::WindowLimiterOverloadControlFilter(): controller_(std::make_unique<WindowLimiterOverloadController>()) {}

std::vector<FilterPoint> WindowLimiterOverloadControlFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // This tracking point is not being used, but tracking points must be paired, so it is added here.
      FilterPoint::SERVER_POST_SCHED_RECV_MSG,
  };
}

void WindowLimiterOverloadControlFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
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

void WindowLimiterOverloadControlFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is already a dirty request, it will not be processed further to ensure that the first error code is
    // not overwritten.
    return;
  }
  // flow control strategy
  if (!controller_->BeforeSchedule(context)) {
    status = FilterStatus::REJECT;
  }
}

}  // namespace trpc::overload_control

#endif
