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
  stop = false;
  refill_thread = std::thread(&TokenBucketOverloadController::refill, this);
}

void Register(const TokenBucketLimiterControlConf& conf) {
  capacity = conf.capacity;
  current_token = conf.current_token;
  rate = conf.rate;
}

bool TokenBucketOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  return true;
}

bool TokenBucketOverloadController::AfterSchedule(const ServerContextPtr& context) {
  return true;
}

void TokenBucketOverloadController::Stop() {
  stop = true;
  if (refill_thread.joinable()) {
    refill_thread.join();
  }
}

void TokenBucketOverloadController::Destroy() {

}

void TokenBucketOverloadController::refill() {
  while (!stop) {
    std::this_thread::sleep_for(std::chrono::millseconds(1000));
    new_token = min(current_token.Load() + rate, capacity);
    current_token.Store(new_token);
  }
}

}  // namespace trpc::overload_control

#endif
