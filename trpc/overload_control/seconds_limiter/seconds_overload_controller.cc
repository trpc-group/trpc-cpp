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

#include "trpc/overload_control/seconds_limiter/seconds_overload_controller.h"
#include <thread>

#include "trpc/overload_control/common/report.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

SecondsOverloadController::SecondsOverloadController(std::string name, int64_t limit, bool is_report,
                          int32_t window_size)
    : name_(std::move(name)), seconds_limiter_(std::make_shared<SecondsLimiter>(limit, is_report, window_size)) {
}

bool SecondsOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  return seconds_limiter_->CheckLimit(context);
}

bool SecondsOverloadController::AfterSchedule(const ServerContextPtr& context) { return false; }

void SecondsOverloadController::Stop() {}

void SecondsOverloadController::Destroy() {}

}  // namespace trpc::overload_control

#endif