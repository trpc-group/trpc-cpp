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

#include "trpc/overload_control/high_percentile/max_in_every_n.h"

#include <algorithm>

namespace trpc::overload_control {

MaxInEveryN::MaxInEveryN(int n) : period_(n), times_(0), max_(0.0) {}

std::pair<bool, double> MaxInEveryN::Sample(double value) {
  max_ = std::max(value, max_);
  ++times_;

  if (times_ >= period_) {
    double max = max_;
    times_ = 0;
    max_ = 0.0;
    return std::pair<bool, double>(true, max);
  }

  return std::pair<bool, double>(false, max_);
}

}  // namespace trpc::overload_control

#endif
