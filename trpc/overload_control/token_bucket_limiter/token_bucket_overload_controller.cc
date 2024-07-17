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

#include "trpc/log/trpc_log.h"

#include "trpc/overload_control/token_bucket_limiter/token_bucket_overload_controller.h"

namespace trpc::overload_control {

bool TokenBucketOverloadController::Init() {
  last_timestamp_ = GetSystemMilliSeconds();
  return true;
}

TokenBucketOverloadController::TokenBucketOverloadController(const TokenBucketLimiterControlConf& conf) {
  capacity_ = conf.capacity;
  current_token_ = conf.initial_token;
  rate_ = conf.rate;
}

bool TokenBucketOverloadController::CheckLimit(uint32_t current_concurrency) {
  return current_concurrency <= current_token_;
}

void TokenBucketOverloadController::AddToken() {
  uint64_t current_timestamp = GetSystemMilliSeconds();
  uint64_t gap_timestamp = current_timestamp - last_timestamp_;
  uint32_t add_count = (uint32_t) (gap_timestamp * rate_ / 1000);
  current_token_ = std::min(current_token_ + add_count, capacity_);
  last_timestamp_ = current_timestamp;
}

void TokenBucketOverloadController::ConsumeToken(uint32_t consume_count) {
  current_token_ -= consume_count;
}

uint32_t TokenBucketOverloadController::GetCurrentToken() {
  return current_token_;
}

bool TokenBucketOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  return true;
}

bool TokenBucketOverloadController::AfterSchedule(const ServerContextPtr& context) {
  return true;
}

void TokenBucketOverloadController::Stop() {

}

void TokenBucketOverloadController::Destroy() {

}

}  // namespace trpc::overload_control

#endif
