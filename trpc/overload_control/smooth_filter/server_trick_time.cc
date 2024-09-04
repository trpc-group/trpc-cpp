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

#include "trpc/overload_control/smooth_filter/server_trick_time.h"

#include <utility>

namespace trpc::overload_control {

TickTime::TickTime(std::chrono::microseconds timeout_us, Function<void()>&& on_timer)
    : timeout_us_(timeout_us), on_timer_(std::move(on_timer)), active_(false) {}

void TickTime::Stop() { Deactivate(); }

void TickTime::Destroy() {}

TickTime::~TickTime() {}

void TickTime::Deactivate() {
  if (active_) {
    active_ = false;
    if (timer_thread_.joinable()) timer_thread_.join();
  }
}

bool TickTime::Init() {
  timer_thread_ = std::thread([this] { Loop(); });
  active_ = true;
  return true;
}

void TickTime::Loop() {
  std::this_thread::sleep_for(timeout_us_);
  while (active_) {
    on_timer_();
    std::this_thread::sleep_for(timeout_us_);
  }
}

}  // namespace trpc::overload_control
#endif
