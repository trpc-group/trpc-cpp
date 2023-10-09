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

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "trpc/overload_control/flow_control/flow_controller.h"

namespace trpc {
namespace overload_control {

/// @brief Flow control factory class.
class FlowControllerFactory {
 public:
  /// @brief Factory singleton.
  static FlowControllerFactory* GetInstance() {
    static FlowControllerFactory instance;
    return &instance;
  }

  // Noncopyable / nonmovable.
  FlowControllerFactory(const FlowControllerFactory&) = delete;
  FlowControllerFactory& operator=(const FlowControllerFactory&) = delete;

  /// @brief Registering a flow controller.
  /// @param name Name of flow controller
  /// @param controller Flow controller
  void Register(const std::string& name, const FlowControllerPtr& controller);

  /// @brief Get a flow controller by name
  /// @param name Search name.
  /// @return Smart pointer of flow controller
  FlowControllerPtr GetFlowController(const std::string& name);

  /// @brief Clear up flow controller
  void Clear();

  /// @brief Get the number of flow controller.
  /// @return size_t type which represents the number of flow controller
  size_t Size() const;

 private:
  FlowControllerFactory() = default;

 private:
  // Store flow controller
  std::unordered_map<std::string, FlowControllerPtr> controller_map_;
};
}  // namespace overload_control

/// @brief Just for compatibility, because the old version is under the "trpc" namespace.
using FlowControllerFactory = overload_control::FlowControllerFactory;
}  // namespace trpc

#endif
