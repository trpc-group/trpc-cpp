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

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "trpc/overload_control/flow_control/flow_controller.h"
#include "trpc/overload_control/flow_control/hit_queue.h"
#include "trpc/overload_control/flow_control/tick_timer.h"

namespace trpc {
namespace overload_control {

/// @brief Default number of time frames per second
constexpr int32_t kDefaultNumFps = 100;

/// @brief Sliding window flow controller.
class SmoothLimiter : public FlowController {
 public:
  explicit SmoothLimiter(int64_t limit, bool is_report = false, int32_t window_size = kDefaultNumFps);

  virtual ~SmoothLimiter();

  // Check if the request is restricted.
  bool CheckLimit(const ServerContextPtr& check_param) override;

  // Check if the request is restricted.
  int64_t GetCurrCounter() override;

  // Get the current maximum request limit of the flow controller.
  int64_t GetMaxCounter() const override { return limit_; }

 protected:
  // Call when the next time slice arrives.
  void OnNextFrame();

 protected:
  // Maximum number of requests per second.
  int64_t limit_;
  // Whether to report monitoring data.
  bool is_report_{false};
  // Window size
  int32_t window_size_{kDefaultNumFps};
  // Hit queue
  HitQueue hit_queue_;
  // Timer implemented using system clock ticks.
  TickTimer tick_timer_;
};
}  // namespace overload_control

/// @brief Just for compatibility, because the old version is under the "trpc" namespace.
using SmoothLimiter = overload_control::SmoothLimiter;
}  // namespace trpc

#endif
