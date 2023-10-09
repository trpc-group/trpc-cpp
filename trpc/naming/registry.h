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

#include <map>
#include <memory>
#include <string>

#include "trpc/common/future/future.h"
#include "trpc/common/plugin.h"
#include "trpc/naming/common/common_defs.h"

namespace trpc {

/// @brief Base class for service registry plugins
class Registry : public Plugin {
 public:
  /// @brief Plugin Type
  PluginType Type() const override { return PluginType::kRegistry; }

  /// @brief Service registration interface.
  /// @param info Information structure related to service registration
  /// @return int Returns 0 on success, -1 on failure
  virtual int Register(const RegistryInfo* info) = 0;

  /// @brief Service unregistration interface.
  /// @param info Information structure related to service registration
  /// @return int Returns 0 on success, -1 on failure
  virtual int Unregister(const RegistryInfo* info) = 0;

  /// @brief Service heartbeat reporting interface.
  /// @param info Information structure related to service registration
  /// @return int Returns 0 on success, -1 on failure
  virtual int HeartBeat(const RegistryInfo* info) = 0;

  /// @brief Asynchronous service heartbeat reporting interface.
  /// @param info Information structure related to service registration
  /// @return Future<> Returns a successful Future with ready state on
  /// success, or a failed Future with exception on failure
  virtual Future<> AsyncHeartBeat(const RegistryInfo* info) = 0;
};

using RegistryPtr = RefPtr<Registry>;

}  // namespace trpc
