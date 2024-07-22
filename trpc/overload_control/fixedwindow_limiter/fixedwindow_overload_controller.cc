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


#include "fixedwindow_overload_controller.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

FixedTimeWindowOverloadController::FixedTimeWindowOverloadController(size_t limit, std::chrono::seconds window)
    : limit_(limit), window_(window), request_count_(0) {
  last_reset_time_ = std::chrono::steady_clock::now();
}

std::string FixedTimeWindowOverloadController::Name() {
  return "FixedTimeWindowOverloadController";
}

bool FixedTimeWindowOverloadController::Init() {
  request_count_ = 0;
  last_reset_time_ = std::chrono::steady_clock::now();
  return true;
}

bool FixedTimeWindowOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::steady_clock::now();
  if (now - last_reset_time_ >= window_) {
    request_count_ = 0;
    last_reset_time_ = now;
  }

  if (request_count_ >= limit_) {
    context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server fixedwindow overload control")); 
    return false;
  }

  ++request_count_;
  return true;
}

bool FixedTimeWindowOverloadController::AfterSchedule(const ServerContextPtr& context) {
  return true;
}

void FixedTimeWindowOverloadController::Stop() {
}

void FixedTimeWindowOverloadController::Destroy() {
}

}  // namespace trpc::overload_control
