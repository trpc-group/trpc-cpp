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
  std::unique_lock<std::mutex> lock(last_alloc_mutex_);
  last_alloc_time_ = trpc::time::GetNanoSeconds();
  return true;
}

void TokenBucketOverloadController::Register(const TokenBucketLimiterControlConf& conf) {
  burst_ = conf.burst;
  rate_ = conf.rate;
  one_token_elapsed_ = nsecs_per_sec_ / rate_;  
  burst_elapsed_ = burst_ * one_token_elapsed_;
}

bool TokenBucketOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  std::unique_lock<std::mutex> lock(last_alloc_mutex_);

  auto now = trpc::time::GetNanoSeconds();
  if(last_alloc_time_ > now - one_token_elapsed_) {
      return false;
  }

  if(last_alloc_time_ < now - burst_elapsed_) {
    last_alloc_time_ = now - burst_elapsed_;
  }
  last_alloc_time_ += one_token_elapsed_;
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
  uint64_t elapsed;
  {
    std::unique_lock<std::mutex> lock(last_alloc_mutex_);
    elapsed = now - last_alloc_time_;
  }

  if(elapsed > burst_elapsed_) {
      elapsed = burst_elapsed_;
  }

  auto sec = elapsed / nsecs_per_sec_;
  auto nsec = elapsed % nsecs_per_sec_;
  return sec * rate_ + nsec * rate_ / nsecs_per_sec_;
}

}  // namespace trpc::overload_control

#endif
