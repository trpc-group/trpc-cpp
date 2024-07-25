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

#include "fixedwindow_limiter_server_filter.h"

#include "fmt/format.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/log/trpc_log.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/overload_control/common/report.h"
#include "trpc/runtime/common/stats/frame_stats.h"

namespace trpc::overload_control {

int FixedTimeWindowServerFilter::Init() {
  bool ok = TrpcConfig::GetInstance()->GetPluginConfig<FixedTimeWindowControlConf>(kOverloadCtrConfField,
                                                                                   kFixedTimeWindowLimiterName, fixedwindow_conf_);
  if (!ok) {
    TRPC_FMT_DEBUG("FixedTimeWindowServerFilter read config failed, will use a default config");
  }

  controller_ = std::make_shared<FixedTimeWindowOverloadController>(fixedwindow_conf_.limit, std::chrono::seconds(fixedwindow_conf_.window_size));
  controller_->Init();

  return 0;
}

std::vector<FilterPoint> FixedTimeWindowServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // This tracking point is not being used, but tracking points must be paired, so it is added here.
      FilterPoint::SERVER_POST_SCHED_RECV_MSG,
  };
}

void FixedTimeWindowServerFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
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

void FixedTimeWindowServerFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is already a dirty request, it will not be processed further to ensure that the first error code is
    // not overwritten.
    return;
  }
  // Get the current unfinished requests.
  const uint32_t current_concurrency = FrameStats::GetInstance()->GetServerStats().GetReqConcurrency();
  bool passed = controller_->BeforeSchedule(context);
  if (!passed) {
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server fixed time window overload control, limit: {}, current concurrency: {}",
                                fixedwindow_conf_.limit, current_concurrency);
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server fixed time window overload control"));
    status = FilterStatus::REJECT;
  }

  if (fixedwindow_conf_.is_report) {
    // Report the judgment result.
    OverloadInfo infos;
    infos.attr_name = kFixedTimeWindowLimiterName;
    infos.report_name = context->GetFuncName();
    infos.tags[kOverloadctrlPass] = (passed == true ? 1 : 0);
    infos.tags[kOverloadctrlLimited] = (passed == false ? 1 : 0);
    infos.tags["current_concurrency"] = current_concurrency;
    infos.tags["limit"] = fixedwindow_conf_.limit;
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
}

}  // namespace trpc::overload_control

#endif
