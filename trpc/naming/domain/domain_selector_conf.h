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

/// @brief domain select plugin configuration
struct DomainSelectorConfig {
  /// @brief Is ipv6 excluded (if the domain is ipv6 only, the exclusion will not apply)
  bool exclude_ipv6{false};

  /// @brief Ciruit break config
  CircuitBreakConfig circuit_break_config;

  /// @brief Print out the logger configuration.
  void Display() const;
};

}  // namespace trpc::naming
