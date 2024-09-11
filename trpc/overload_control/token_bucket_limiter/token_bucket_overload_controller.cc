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
  std::unique_lock<std::mutex> lock(lr_mutex);
  last_request = trpc::time::GetSystemNanoSeconds();

  return true;
}

void TokenBucketOverloadController::Register(const TokenBucketLimiterControlConf& conf) {
  burst = conf.burst;
  rate = conf.rate;
  spend = nsecs_per_sec / rate;  
  max_elapsed = burst * spend;
}

bool TokenBucketOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  std::unique_lock<std::mutex> lock{lr_mutex};

  auto now{trpc::time::GetSystemNanoSeconds()};
  auto elapsed{now - last_request};
  if(elapsed > max_elapsed) {
      elapsed = max_elapsed;
  }

  elapsed += spend;
  if(elapsed > now) {
    return false;
  }
  last_request = now;

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
  return burst;
}

uint64_t TokenBucketOverloadController::GetRemainingTokens(uint64_t now) {
  auto elapsed{now};
  {
    std::unique_lock<std::mutex> lock{lr_mutex};
    elapsed -= last_request;
  }

  if(elapsed > max_elapsed) {
      elapsed = max_elapsed;
  }

  auto sec{elapsed / nsecs_per_sec};
  auto nsec{elapsed % nsecs_per_sec};
  return sec * rate + nsec * rate / nsecs_per_sec;
}

}  // namespace trpc::overload_control

#endif
