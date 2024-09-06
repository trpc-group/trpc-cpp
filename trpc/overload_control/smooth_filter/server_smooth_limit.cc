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
#include <chrono>
#include <cmath>
#include <cstdint>

#include "trpc/overload_control/smooth_filter/server_smooth_limit.h"
#include "trpc/overload_control/common/report.h"

#include "trpc/util/log/logging.h"

namespace trpc::overload_control {

bool SmoothLimit::Init() {
  // nothing to do,The time thread automatically starts.
  return true;
}

bool SmoothLimit::BeforeSchedule(const ServerContextPtr& context) {
    return smooth_limit_.CheckLimit(context);
}

void SmoothLimit::Destroy(){
    // nothing to do,The time thread automatically destorys.
}

void SmoothLimit::Stop(){
    // nothing to do,The time thread automatically stops.
};

SmoothLimit::SmoothLimit(std::string name,int64_t limit, bool is_report, int32_t window_size)
    : name_(name),
      smooth_limit_(limit, is_report, window_size) {
}

//The destructor does nothing, but the stop method in public needs to set the timed task to join and make it invalid
//The user must stop before calling destroy
SmoothLimit::~SmoothLimit() {}

}  // namespace trpc::overload_control
#endif

