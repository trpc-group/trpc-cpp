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

#include "trpc/overload_control/sliding_window_limiter/sliding_window_overload_controller.h"

namespace trpc::overload_control {

SlidingWindowOverloadController::SlidingWindowOverloadController(int64_t limit, bool is_report)
    : smooth_limter_(std::make_unique<SmoothLimiter>(limit, is_report)) {}

bool SlidingWindowOverloadController::Init() { return true; }

bool SlidingWindowOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  return !smooth_limter_->CheckLimit(context);
}

bool SlidingWindowOverloadController::AfterSchedule(const ServerContextPtr& context) { return true; }

void SlidingWindowOverloadController::Stop() {}

void SlidingWindowOverloadController::Destroy() {}

}  // namespace trpc::overload_control

#endif
