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

#pragma once

#include "trpc/tvar/compound_ops/internal_latency.h"

namespace trpc::fiber::detail {

struct SchedulingVar {
  static SchedulingVar* GetInstance() {
    static SchedulingVar instance;
    return &instance;
  }

  SchedulingVar(const SchedulingVar&) = delete;
  SchedulingVar& operator=(const SchedulingVar&) = delete;

  tvar::internal::InternalLatencyInTsc ready_run_latency{"trpc/fiber/latency/ready_to_run"};

 private:
  SchedulingVar() = default;
};

}  // namespace trpc::fiber::detail
