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


#include "slidingwindow_overload_controller.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

SlidingWindowOverloadController::SlidingWindowOverloadController(size_t max_requests, std::chrono::seconds window_duration)
    : max_requests_(max_requests), window_duration_(window_duration) {
}

std::string SlidingWindowOverloadController::Name() {
  return "SlidingWindowOverloadController";
}

bool SlidingWindowOverloadController::Init() {
  return true;
}

bool SlidingWindowOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto now = std::chrono::steady_clock::now();
  while(!timestamps_.empty() && (now - timestamps_.front()) > window_duration_) {
    timestamps_.pop_front();
  }

  if(timestamps_.size() < max_requests_) {
    timestamps_.push_back(now);
    return true;
  } else {
    context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server slidingwindow overload control")); 
    return false;
  }
}

bool SlidingWindowOverloadController::AfterSchedule(const ServerContextPtr& context) {
  return true;
}

void SlidingWindowOverloadController::Stop() {
}

void SlidingWindowOverloadController::Destroy() {
}

}  // namespace trpc::overload_control

#endif