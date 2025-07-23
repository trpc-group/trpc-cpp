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

#pragma once

#include <cstdint>
#include <memory>

#include "trpc/server/server_context.h"

namespace trpc {
namespace overload_control {

/// @brief Base class of Flow control.
class FlowController {
 public:
  virtual ~FlowController() = default;

  /// @brief Check if the request is restricted.
  /// @param context Server Context
  /// @return bool true - Overloaded, false: Not overloaded.
  virtual bool CheckLimit(const ServerContextPtr& context) = 0;

  /// @brief Get the total number of current requests.
  /// @return The value of the counter.
  virtual int64_t GetCurrCounter() = 0;

  /// @brief Get the current maximum request limit of the flow controller.
  /// @return Return the maximum value of the maximum request limit.
  virtual int64_t GetMaxCounter() const = 0;
};

using FlowControllerPtr = std::shared_ptr<FlowController>;
}  // namespace overload_control

/// @brief Just for compatibility, because the old version is under the "trpc" namespace.
using FlowControllerPtr = overload_control::FlowControllerPtr;

}  // namespace trpc

#endif
