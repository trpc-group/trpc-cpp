/*
 *
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2024 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/token_bucket_limiter/token_bucket_overload_controller.h"

#include "trpc/log/trpc_log.h"
#include "trpc/overload_control/common/report.h"

namespace trpc::overload_control {

TokenBucketOverloadController::TokenBucketOverloadController(uint64_t burst, uint64_t rate, bool is_report)
    : burst_(burst), rate_(std::max(rate, min_rate_)), is_report_(is_report) {
  one_token_elapsed_ = nsecs_per_sec_ / rate_;
  burst_elapsed_ = burst_ * one_token_elapsed_;
}

bool TokenBucketOverloadController::Init() {
  last_alloc_time_ = trpc::time::GetSteadyNanoSeconds();
  return true;
}

bool TokenBucketOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  bool ret = true;
  uint64_t now, last_alloc_time;
  {
    std::unique_lock<std::mutex> lock(last_alloc_mutex_);

    now = trpc::time::GetSteadyNanoSeconds();
    if (last_alloc_time_ > now - one_token_elapsed_) {
      ret = false;
    } else if (last_alloc_time_ < now - burst_elapsed_) {
      last_alloc_time_ = now - burst_elapsed_ + one_token_elapsed_;
    } else {
      last_alloc_time_ += one_token_elapsed_;
    }
    last_alloc_time = last_alloc_time_;
  }

  if (is_report_) {
    auto elapsed = now - last_alloc_time;
    if (elapsed > burst_elapsed_) {
      elapsed = burst_elapsed_;
    }
    auto sec = elapsed / nsecs_per_sec_;
    auto nsec = elapsed % nsecs_per_sec_;
    auto remaining_tokens = sec * rate_ + nsec * rate_ / nsecs_per_sec_;

    OverloadInfo infos;
    infos.attr_name = kOverloadctrlTokenBucketLimiter;
    infos.report_name = context->GetFuncName();
    infos.tags["burst"] = burst_;
    infos.tags["remaining_tokens"] = remaining_tokens;
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
  return ret;
}

bool TokenBucketOverloadController::AfterSchedule(const ServerContextPtr& context) { return true; }

void TokenBucketOverloadController::Stop() {}

void TokenBucketOverloadController::Destroy() {}

// uint64_t TokenBucketOverloadController::GetRemainingTokens(uint64_t now) {
//   uint64_t elapsed;
//   {
//     std::unique_lock<std::mutex> lock(last_alloc_mutex_);
//     elapsed = now - last_alloc_time_;
//   }

//   if (elapsed > burst_elapsed_) {
//     elapsed = burst_elapsed_;
//   }

//   auto sec = elapsed / nsecs_per_sec_;
//   auto nsec = elapsed % nsecs_per_sec_;
//   return sec * rate_ + nsec * rate_ / nsecs_per_sec_;
// }

}  // namespace trpc::overload_control

#endif
