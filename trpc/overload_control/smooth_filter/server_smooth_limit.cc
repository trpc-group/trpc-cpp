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
#include <chrono>
#include <cmath>
#include <cstdint>

#include "trpc/overload_control/smooth_filter/server_smooth_limit.h"
#include "trpc/overload_control/common/report.h"

#include "trpc/util/log/logging.h"

namespace trpc::overload_control {

bool SmoothLimit::Init() {
    return tick_timer_.Init();
}

bool SmoothLimit::BeforeSchedule(const ServerContextPtr& context) {
   
    return CheckLimit(context);
}



void SmoothLimit::Destroy(){
    tick_timer_.Destroy();
}

void SmoothLimit::Stop(){
    tick_timer_.Stop();
};



SmoothLimit::SmoothLimit(std::string name,int64_t limit, bool is_report, int32_t window_size)
    : name_(name),
      limit_(limit),
      is_report_(is_report),
      window_size_(window_size <= 0 ? kDefaultNum : window_size),
      requestrollque_(window_size_),
      tick_timer_(std::chrono::microseconds(static_cast<int64_t>(std::ceil(1000000.0 / window_size_))),
                  [this] { OnNextTimeFrame(); }) {
  TRPC_ASSERT(window_size_ > 0);
}

//The destructor does nothing, but the stop method in public needs to set the timed task to join and make it invalid
//The user must stop before calling destroy
SmoothLimit::~SmoothLimit() {}

//Check if the current limit is exceeded
bool SmoothLimit::CheckLimit(const ServerContextPtr& context) {
  bool ret = false;
  int64_t active_sum = requestrollque_.ActiveSum();
  
  int64_t hit_num = 0;
  if (active_sum >= limit_) {
    ret = true;
  } else {
    hit_num = requestrollque_.AddTimeHit();
    
    
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
    infos.tags["window_size"] = window_size_;
    infos.tags[kOverloadctrlPass] = (ret ? 0 : 1);
    infos.tags[kOverloadctrlLimited] = (ret ? 1 : 0);
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
  return ret;
}

int64_t SmoothLimit::GetCurrCounter() { return requestrollque_.ActiveSum(); }

void SmoothLimit::OnNextTimeFrame() { requestrollque_.NextTimeFrame(); }

}  // namespace trpc::overload_control
#endif

