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

#include "trpc/overload_control/high_percentile/high_avg_ema.h"

#include <algorithm>
#include <cmath>

namespace trpc::overload_control {

HighAvgEMA::HighAvgEMA(const Options& options)
    : inc_factor(std::min(1.0, std::max(0.0, options.inc_factor))),
      dec_factor(std::min(1.0, std::max(0.0, options.dec_factor))),
      val_(0.0) {}

double HighAvgEMA::UpdateAndGet(double sample_val) {
  // Handling boundary values.
  if (!std::isfinite(val_)) {
    val_ = sample_val;
    return val_;
  }

  double factor = (sample_val >= val_) ? inc_factor : dec_factor;
  val_ = sample_val * factor + val_ * (1 - factor);

  return val_;
}

double HighAvgEMA::Get() { return val_; }

}  // namespace trpc::overload_control

#endif
