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
#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "trpc/util/function.h"

namespace trpc::overload_control {
class TickTime {
 public:
  /// @brief Create a timer with a specified timeout interval and callback function,
  /// and automatically activate the timer immediately after creation.
  /// @param timeout_us The time interval (in microseconds) triggered by the timer callback.
  /// @param on_timer The callback function called when the scheduled time is reached.

  explicit TickTime(std::chrono::microseconds timeout_us, Function<void()>&& on_timer);

  virtual ~TickTime();

  void Deactivate();

  void Destroy();

  void Stop();

  bool Init();

 private:
  void Loop();

 private:
  const std::chrono::microseconds timeout_us_;

  /// @param Callback function, calling NextFrame() at regular intervals
  const Function<void()> on_timer_;

  /// @param Controlling the loop
  std::atomic<bool> active_;
  /// @param Task of opening a separate thread to execute callback functions
  std::thread timer_thread_;
};
}  // namespace trpc::overload_control

#endif