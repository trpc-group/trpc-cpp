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

#include <cstdint>
#include <memory>
#include <string>

#include "trpc/overload_control/flow_control/flow_controller.h"
#include "trpc/overload_control/flow_control/recent_queue.h"

namespace trpc {
namespace overload_control {

/// @brief Default number of time frames per second
constexpr int32_t kDefaultNumFps = 100;

/// @brief Sliding window flow controller.
class SmoothLimiter : public FlowController {
 public:
  explicit SmoothLimiter(int64_t limit, bool is_report = false, int32_t window_size = kDefaultNumFps);

  // Check if the request is restricted.
  bool CheckLimit(const ServerContextPtr& check_param) override;

  // Check if the request is restricted.
  int64_t GetCurrCounter() override;

  // Get the current maximum request limit of the flow controller.
  int64_t GetMaxCounter() const override { return limit_; }

 protected:
  // Maximum number of requests per second.
  int64_t limit_;
  // Whether to report monitoring data.
  bool is_report_{false};
  // Window size
  int32_t window_size_{kDefaultNumFps};

  RecentQueuePtr recent_queue_;
  // Nanoseconds per second, 1s = 10^9 ns
  static constexpr auto nsecs_per_sec_ = static_cast<uint64_t>(1e9);
};
}  // namespace overload_control

/// @brief Just for compatibility, because the old version is under the "trpc" namespace.
using SmoothLimiter = overload_control::SmoothLimiter;
}  // namespace trpc

#endif
