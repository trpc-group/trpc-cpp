/*
*
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2023 THL A29 Limited, a Tencent company.
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

#include <mutex>
#include <chrono>
#include <memory>
#include <optional>

#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_conf.h"
#include "trpc/overload_control/server_overload_controller.h"
#include "trpc/server/server_context.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/tvar/common/atomic_type.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

/// @brief Overload protection controller based on token bucket algorithm.
class TokenBucketOverloadController : public ServerOverloadController {
public:
  /// @brief Register the controller plugin by the config.
  void Register(const TokenBucketLimiterControlConf& conf);

  /// @brief Name of controller
  std::string Name() override { return "TokenBucketOverloadController"; };

  /// @brief Initialization function.
  bool Init() override;

  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

  uint64_t GetRemainingTokens();

private:
  // Maximum of burst size.
  uint64_t burst;
  // The rate(tokens/second) of token generation.
  uint64_t rate;

  // The time when the last request was successfully passed.
  uint64_t last_request;
  std::mutex lr_mutex;

  // The time(nanoseconds) of one token generation.
  uint64_t spend;
  // The maximum time(nanoseconds) required for the size of tokens to reach burst.
  uint64_t max_elapsed;

  static constexpr auto nsecs_per_sec{static_cast<uint64_t>(1e9)};
};

using TokenBucketOverloadControllerPtr = std::shared_ptr<TokenBucketOverloadController>;

}  // namespace trpc::overload_control

#endif
