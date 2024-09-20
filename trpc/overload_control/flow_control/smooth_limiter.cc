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

#include "trpc/overload_control/flow_control/smooth_limiter.h"
#include "trpc/overload_control/common/report.h"

namespace trpc::overload_control {

SmoothLimiter::SmoothLimiter(int64_t limit, bool is_report, int32_t window_size)
    : limit_(limit),
      is_report_(is_report),
      window_size_(window_size <= 0 ? kDefaultNumFps : window_size),
      recent_queue_(std::make_unique<RecentQueue>(limit, nsecs_per_sec_)) {
  TRPC_ASSERT(window_size_ > 0);
}

bool SmoothLimiter::CheckLimit(const ServerContextPtr& context) {
  bool ret = !recent_queue_->Add();

  if (is_report_) {
    OverloadInfo infos;
    infos.attr_name = "SmoothLimiter";
    infos.report_name = context->GetFuncName();
    infos.tags["active_count"] = recent_queue_->ActiveCount();
    infos.tags["max_qps"] = limit_;
    infos.tags[kOverloadctrlPass] = (ret ? 0 : 1);
    infos.tags[kOverloadctrlLimited] = (ret ? 1 : 0);
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
  return ret;
}

int64_t SmoothLimiter::GetCurrCounter() { return recent_queue_->ActiveCount(); }

}  // namespace trpc::overload_control

#endif
