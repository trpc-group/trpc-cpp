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

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "trpc/util/function.h"

namespace trpc::overload_control {

/// @brief Timer triggered by system clock ticks.
/// @note 1.Pass the time interval and callback function during construction.
//        2.Start a thread and call the processing function when the time interval is reached.
class TickTimer {
 public:
  /// @brief Create a timer with a specified timeout interval and callback function, and activate the timer
  ///        automatically immediately after creation.
  /// @param timeout_us Time interval in microseconds for timer callback triggering.
  /// @param on_timer Callback this function when the scheduled time is reached.
  explicit TickTimer(std::chrono::microseconds timeout_us, Function<void()>&& on_timer);

  virtual ~TickTimer();

  /// @brief Stop timer
  void Deactivate();

 private:
  // Timer thread loop.
  void Loop();

 private:
  // Timeout interval time in microseconds for the timer.
  const std::chrono::microseconds timeout_us_;
  // Callback this function on timer expiration.
  const Function<void()> on_timer_;
  // Flag for timer thread activation.
  std::atomic<bool> active_;
  // Timer thread.
  std::thread timer_thread_;
};

using TickTimerPtr = std::shared_ptr<TickTimer>;

}  // namespace trpc::overload_control

#endif
