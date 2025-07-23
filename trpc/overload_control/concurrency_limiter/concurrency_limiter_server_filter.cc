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

#include "trpc/overload_control/concurrency_limiter/concurrency_limiter_server_filter.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/log/trpc_log.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/overload_control/common/report.h"
#include "trpc/runtime/common/stats/frame_stats.h"

namespace trpc::overload_control {

int ConcurrencyLimiterServerFilter::Init() {
  bool ok = TrpcConfig::GetInstance()->GetPluginConfig<ConcurrencyLimiterControlConf>(
      kOverloadCtrConfField, kConcurrencyLimiterName, concurrency_conf_);
  if (!ok) {
    TRPC_FMT_DEBUG("ConcurrencyLimiterServerFilter read config failed, will use a default config");
  }
  concurrency_conf_.Display();

  return 0;
}

std::vector<FilterPoint> ConcurrencyLimiterServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // This tracking point is not used, but tracking points must be paired, so it is added here.
      FilterPoint::SERVER_POST_SCHED_RECV_MSG,
  };
}

void ConcurrencyLimiterServerFilter::operator()(FilterStatus& status, FilterPoint point,
                                                const ServerContextPtr& context) {
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

void ConcurrencyLimiterServerFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is already a dirty request, it will not be processed further to ensure that the first error code is
    // not overwritten.
    return;
  }
  // Get the current unfinished requests.
  const uint32_t current_concurrency = FrameStats::GetInstance()->GetServerStats().GetReqConcurrency();
  // Compare the current unfinished requests with the maximum concurrency to determine whether interception is required.
  bool passed = (current_concurrency <= concurrency_conf_.max_concurrency);
  if (!passed) {
    TRPC_FMT_ERROR_EVERY_SECOND(
        "rejected by concurrency limiter overload control, current concurrency: {}, max concurrency: {}",
        current_concurrency, concurrency_conf_.max_concurrency);
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by concurrency limiter overload control"));
    status = FilterStatus::REJECT;
  }
  // Report the result.
  if (concurrency_conf_.is_report) {
    OverloadInfo infos;
    infos.attr_name = kOverloadctrlConcurrencyLimiter;
    infos.report_name = fmt::format("/{}/{}", context->GetCalleeName(), context->GetFuncName());
    infos.tags[kOverloadctrlPass] = (passed == true ? 1 : 0);
    infos.tags[kOverloadctrlLimited] = (passed == false ? 1 : 0);
    infos.tags["max_concurrency"] = concurrency_conf_.max_concurrency;
    infos.tags["current_concurrency"] = current_concurrency;
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
}

}  // namespace trpc::overload_control

#endif
