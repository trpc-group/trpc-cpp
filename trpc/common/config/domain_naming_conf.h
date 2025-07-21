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

namespace trpc::naming {

/// @brief domain select plugin configuration
struct DomainSelectorConfig {
  /// @brief Is ipv6 excluded (if the domain is ipv6 only, the exclusion will not apply)
  bool exclude_ipv6{false};

  /// @brief Print out the logger configuration.
  void Display() const;
};

}  // namespace trpc::naming
