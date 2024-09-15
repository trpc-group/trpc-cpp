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

#include "trpc/overload_control/fixed_window_limiter/fixed_window_limiter_conf.h"
#include "trpc/overload_control/flow_control/seconds_limiter.h"
#include "trpc/overload_control/server_overload_controller.h"
#include "trpc/server/server_context.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

/// @brief Overload protection controller based on fixed window algorithm.
class FixedWindowOverloadController : public ServerOverloadController {
 public:
  explicit FixedWindowOverloadController(int64_t limit, bool is_report);

  /// @brief Name of controller
  std::string Name() override { return "FixedWindowOverloadController"; };

  /// @brief Initialization function.
  bool Init() override;

  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

 private:
  std::unique_ptr<SecondsLimiter> seconds_limter_;
};

using FixedWindowOverloadControllerPtr = std::unique_ptr<FixedWindowOverloadController>;

}  // namespace trpc::overload_control

#endif
