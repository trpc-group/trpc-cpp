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

#include <chrono>
#include <cmath>
#include <cstdint>

#include "trpc/overload_control/common/report.h"
#include "trpc/util/log/logging.h"

namespace trpc::overload_control {

SmoothLimiter::SmoothLimiter(int64_t limit, bool is_report, int32_t window_size)
    : limit_(limit),
      is_report_(is_report),
      window_size_(window_size <= 0 ? kDefaultNumFps : window_size),
      hit_queue_(window_size_),
      tick_timer_(std::chrono::microseconds(static_cast<int64_t>(std::ceil(1000000.0 / window_size_))),
                  [this] { OnNextFrame(); }) {
  TRPC_ASSERT(window_size_ > 0);
}

SmoothLimiter::~SmoothLimiter() { tick_timer_.Deactivate(); }

bool SmoothLimiter::CheckLimit(const ServerContextPtr& context) {
  bool ret = false;
  int64_t active_sum = hit_queue_.ActiveSum();
  int64_t hit_num = 0;
  if (active_sum >= limit_) {
    ret = true;
  } else {
    hit_num = hit_queue_.AddHit();
    if (hit_num > limit_) {
      ret = true;
    }
  }
  if (is_report_) {
    OverloadInfo infos;
    infos.attr_name = "SmoothLimiter";
    infos.report_name = context->GetFuncName();
    infos.tags["active_sum"] = active_sum;
    infos.tags["hit_num"] = hit_num;
    infos.tags["max_qps"] = limit_;
    infos.tags[kOverloadctrlPass] = (ret ? 0 : 1);
    infos.tags[kOverloadctrlLimited] = (ret ? 1 : 0);
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
  return ret;
}

int64_t SmoothLimiter::GetCurrCounter() { return hit_queue_.ActiveSum(); }

void SmoothLimiter::OnNextFrame() { hit_queue_.NextFrame(); }

}  // namespace trpc::overload_control

#endif
