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

#include "trpc/overload_control/throttler/throttler_ema.h"

namespace trpc::overload_control {

ThrottlerEma::ThrottlerEma(const Options& options)
    : options_(options), last_(std::chrono::steady_clock::now()), total_(0.0) {}

void ThrottlerEma::Add(std::chrono::steady_clock::time_point time_point, double value) {
  Advance(time_point);
  total_ += value;
}

double ThrottlerEma::CurrentTotal() const { return total_; }

double ThrottlerEma::Sum(std::chrono::steady_clock::time_point time_point) {
  Advance(time_point);
  return total_;
}

void ThrottlerEma::Advance(std::chrono::steady_clock::time_point time_point) {
  // If the time since the last data is too long, it is considered that there is no existing data.
  if (time_point - last_ > options_.reset_interval) {
    total_ = 0.0;
    last_ = time_point;
    return;
  }

  while (time_point - last_ > options_.interval) {
    total_ *= options_.factor;
    last_ += options_.interval;
  }
}

}  // namespace trpc::overload_control

#endif
