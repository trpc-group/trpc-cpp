
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

#pragma once

#include "trpc/naming/common/util/circuit_break/circuit_breaker_config.h"

namespace trpc::naming {

struct DirectSelectorConfig {
  CircuitBreakConfig circuit_break_config;

  void Display() const { circuit_break_config.Display(); }
};

}  // namespace trpc::naming
