/*
*
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2023 THL A29 Limited, a Tencent company.
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

#include <atomic>

#include "trpc/log/trpc_log.h"

// using namespace std::literals;

namespace trpc::overload_control {

bool TokenBucketOverloadController::Init() {
  std::unique_lock<std::mutex> lock(lr_mutex_);
  last_request_ = trpc::time::GetSystemNanoSeconds();

  return true;
}

void TokenBucketOverloadController::Register(const TokenBucketLimiterControlConf& conf) {
  burst_ = conf.burst;
  rate_ = conf.rate;
  spend_ = nsecs_per_sec_ / rate_;  
  max_elapsed_ = burst_ * spend_;
}

bool TokenBucketOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  std::unique_lock<std::mutex> lock{lr_mutex_};

  auto now{trpc::time::GetSystemNanoSeconds()};
  auto elapsed{now - last_request_};
  if(elapsed > max_elapsed_) {
      elapsed = max_elapsed_;
  }

  elapsed += spend_;
  if(elapsed > now) {
    return false;
  }
  last_request_ = now;

  return true;
}

bool TokenBucketOverloadController::AfterSchedule(const ServerContextPtr& context) {
  return true;
}

void TokenBucketOverloadController::Stop() {

}

void TokenBucketOverloadController::Destroy() {

}

uint64_t TokenBucketOverloadController::GetBurst() {
  return burst_;
}

uint64_t TokenBucketOverloadController::GetRemainingTokens(uint64_t now) {
  auto elapsed{now};
  {
    std::unique_lock<std::mutex> lock{lr_mutex_};
    elapsed -= last_request_;
  }

  if(elapsed > max_elapsed_) {
      elapsed = max_elapsed_;
  }

  auto sec{elapsed / nsecs_per_sec_};
  auto nsec{elapsed % nsecs_per_sec_};
  return sec * rate_ + nsec * rate_ / nsecs_per_sec_;
}

}  // namespace trpc::overload_control

#endif
