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

#include "trpc/overload_control/token_bucket_limiter/token_bucket_overload_controller.h"

namespace trpc::overload_control {

bool TokenBucketOverloadController::Init() {
  begin_timestamp = GetSystemMilliSeconds();
  return true;
}

TokenBucketOverloadController::TokenBucketOverloadController(const TokenBucketLimiterControlConf& conf) {
  capacity = conf.capacity;
  current_token = conf.current_token;
  rate = conf.rate;
}

void TokenBucketOverloadController::Register(const TokenBucketLimiterControlConf& conf) {
  capacity = conf.capacity;
  current_token = conf.current_token;
  rate = conf.rate;
}

bool TokenBucketOverloadController::CheckLimit(uint32_t current_concurrency) {
  return current_concurrency <= current_token;
}

void TokenBucketOverloadController::AddToken() {
  uint64_t current_timestamp = GetSystemMilliSeconds();
  uint64_t gap_timestamp = current_timestamp - begin_timestamp;
  uint32_t add_count = (uint32_t) (gap_timestamp * rate / 1000);
  current_token = std::min(current_token + add_count, capacity);
}

void TokenBucketOverloadController::ConsumeToken(uint32_t consume_count) {
  current_token -= consume_count;
}

uint64_t TokenBucketOverloadController::GetBeginTimeStamp() {
  return begin_timestamp;
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
