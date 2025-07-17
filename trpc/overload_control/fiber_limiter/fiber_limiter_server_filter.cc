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

#include "trpc/overload_control/fiber_limiter/fiber_limiter_server_filter.h"

#include "fmt/format.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/log/trpc_log.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/overload_control/common/report.h"
#include "trpc/runtime/fiber_runtime.h"

namespace trpc::overload_control {

int FiberLimiterServerFilter::Init() {
  bool ok = TrpcConfig::GetInstance()->GetPluginConfig<FiberLimiterControlConf>(kOverloadCtrConfField,
                                                                                kFiberLimiterName, fiber_limiter_conf_);
  if (!ok) {
    TRPC_FMT_DEBUG("FiberLimiterServerFilter read config failed, will use a default config");
  }

  return 0;
}

std::vector<FilterPoint> FiberLimiterServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // This tracking point is not being used, but tracking points must be paired, so it is added here.
      FilterPoint::SERVER_POST_SCHED_RECV_MSG,
  };
}

void FiberLimiterServerFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
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

void FiberLimiterServerFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is already a dirty request, it will not be processed further to ensure that the first error code is
    // not overwritten.
    return;
  }
  // Get the number of fibers that are currently not yet processed.
  const std::size_t fibers_count = trpc::fiber::GetFiberQueueSize();
  // Compare the number of fibers that are currently not yet processed with the maximum concurrent number of fibers to
  // determine whether interception is necessary.
  bool passed = (fibers_count <= fiber_limiter_conf_.max_fiber_count);
  if (!passed) {
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server fiber limiter overload control, fibers_count: {}, max fibers: {}",
                                fibers_count, fiber_limiter_conf_.max_fiber_count);
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server fiber limiter overload control"));
    status = FilterStatus::REJECT;
  }

  if (fiber_limiter_conf_.is_report) {
    // Report the judgment result.
    OverloadInfo infos;
    infos.attr_name = kOverloadctrlFiberLimiter;
    infos.report_name = context->GetFuncName();
    infos.tags[kOverloadctrlPass] = (passed == true ? 1 : 0);
    infos.tags[kOverloadctrlLimited] = (passed == false ? 1 : 0);
    infos.tags["fibers_count"] = fibers_count;
    infos.tags["max_fiber_count"] = fiber_limiter_conf_.max_fiber_count;
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
}

}  // namespace trpc::overload_control

#endif
