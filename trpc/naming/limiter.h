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

#include <map>
#include <memory>
#include <string>

#include "trpc/common/plugin.h"
#include "trpc/naming/common/common_defs.h"

namespace trpc {

/// @brief Base class for limiter plugins
class Limiter : public Plugin {
 public:
  /// @brief Plugin Type
  PluginType Type() const override { return PluginType::kLimiter; }

  /// @brief Interface for getting the limit status, used to determine whether to limit
  /// @param info Service limit information structure
  /// @return LimitRetCode Limit result return code
  virtual LimitRetCode ShouldLimit(const LimitInfo* info) = 0;

  /// @brief Interface for finishing limit processing, such as reporting some limit data
  /// @param result Limit result structure
  /// @return int Returns 0 on success, -1 on failure
  virtual int FinishLimit(const LimitResult* result) = 0;
};

using LimiterPtr = RefPtr<Limiter>;

}  // namespace trpc
