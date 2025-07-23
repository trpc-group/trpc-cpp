//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/flow_control/tick_timer.h"

#include <utility>

namespace trpc::overload_control {

TickTimer::TickTimer(std::chrono::microseconds timeout_us, Function<void()>&& on_timer)
    : timeout_us_(timeout_us), on_timer_(std::move(on_timer)), active_(true) {
  timer_thread_ = std::thread([this] { Loop(); });
}

TickTimer::~TickTimer() { Deactivate(); }

void TickTimer::Deactivate() {
  if (active_) {
    active_ = false;
    if (timer_thread_.joinable()) timer_thread_.join();
  }
}

void TickTimer::Loop() {
  std::this_thread::sleep_for(timeout_us_);
  while (active_) {
    on_timer_();
    std::this_thread::sleep_for(timeout_us_);
  }
}

}  // namespace trpc::overload_control

#endif
