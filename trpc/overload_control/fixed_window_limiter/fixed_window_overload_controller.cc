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

#include "trpc/overload_control/fixed_window_limiter/fixed_window_overload_controller.h"

namespace trpc::overload_control {

FixedWindowOverloadController::FixedWindowOverloadController(int64_t limit, bool is_report)
    : seconds_limter_(std::make_unique<SecondsLimiter>(limit, is_report)) {}

bool FixedWindowOverloadController::Init() { return true; }

bool FixedWindowOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  return !seconds_limter_->CheckLimit(context);
}

bool FixedWindowOverloadController::AfterSchedule(const ServerContextPtr& context) { return true; }

void FixedWindowOverloadController::Stop() {}

void FixedWindowOverloadController::Destroy() {}

}  // namespace trpc::overload_control

#endif
