/*
 *
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2024 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

#include "trpc/overload_control/server_overload_controller.h"
#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_conf.h"
#include "trpc/server/server_context.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

/// @brief Overload protection controller based on token bucket algorithm.
class TokenBucketOverloadController : public ServerOverloadController {
 public:
  explicit TokenBucketOverloadController(uint64_t burst, uint64_t rate, bool is_report);

  /// @brief Name of controller
  std::string Name() override { return "TokenBucketOverloadController"; };

  /// @brief Initialization function.
  bool Init() override;

  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

 private:
  // Maximum of burst size.
  uint64_t burst_;
  // The rate(tokens/second) of token generation.
  uint64_t rate_;
  // Whether to report monitoring data.
  bool is_report_;

  // The last time when the token was allocated.
  uint64_t last_alloc_time_;
  // Protect last_alloc_time_ for concurrent read/write.
  std::mutex last_alloc_mutex_;

  // The time(nanoseconds) of one token generation.
  uint64_t one_token_elapsed_;
  // The time(nanoseconds) of burst_ tokens generation.
  uint64_t burst_elapsed_;

  // Minimum rate(tokens/second) of token generation.
  static constexpr uint64_t min_rate_ = 1;

  // Nanoseconds per second, 1s = 10^9 ns
  static constexpr auto nsecs_per_sec_ = static_cast<uint64_t>(1e9);
};

using TokenBucketOverloadControllerPtr = std::unique_ptr<TokenBucketOverloadController>;

}  // namespace trpc::overload_control

#endif
