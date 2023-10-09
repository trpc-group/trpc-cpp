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
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "trpc/overload_control/flow_control/flow_controller.h"

namespace trpc {
namespace overload_control {

/// @brief Default number of time frames per second
constexpr int32_t kDefaultWindowSize = 10;

/// @brief Fixed Window Flow Controller
class SecondsLimiter : public FlowController {
 public:
  explicit SecondsLimiter(int64_t limit, bool is_report = false, int32_t window_size = kDefaultWindowSize);

  // Check if the request is restricted.
  bool CheckLimit(const ServerContextPtr& check_param) override;

  // Check if the request is restricted.
  int64_t GetCurrCounter() override;

  // Get the current maximum request limit of the flow controller.
  int64_t GetMaxCounter() const override { return limit_; }

 private:
  // Maximum number of requests per second.
  int64_t limit_{0};
  // Whether to report monitoring data.
  bool is_report_{false};
  // Window size
  int32_t window_size_{kDefaultWindowSize};
  struct SecondsCounter {
    std::atomic<int64_t> counter{0};
    std::atomic<int64_t> access_timestamp{0};
  };
  // Counter
  std::unique_ptr<SecondsCounter[]> counters_;
  // The time when the last request was successfully passed.
  std::atomic<int64_t> last_access_time_{0};
  // Mutex lock to ensure thread safety.
  std::mutex mutex_;
};
}  // namespace overload_control

/// @brief Just for compatibility, because the old version is under the "trpc" namespace.
using SecondsLimiter = overload_control::SecondsLimiter;

}  // namespace trpc

#endif
