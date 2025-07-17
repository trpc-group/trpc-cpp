//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/high_percentile/high_avg_strategy.h"

namespace trpc::overload_control {

HighAvgStrategy::HighAvgStrategy(const Options& options) : options_(options) {}

const std::string& HighAvgStrategy::Name() { return options_.name; }

double HighAvgStrategy::GetExpected() const { return options_.expected; }

double HighAvgStrategy::GetMeasured() { return high_avg_.AdvanceAndGet(); }

void HighAvgStrategy::Sample(double value) { high_avg_.Sample(value); }

}  // namespace trpc::overload_control

#endif
