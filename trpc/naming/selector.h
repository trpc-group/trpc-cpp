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

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "trpc/common/future/future.h"
#include "trpc/common/plugin.h"
#include "trpc/naming/common/common_defs.h"

namespace trpc {

/// @brief Base class for service discovery
class Selector : public Plugin {
 public:
  /// @brief Plugin name
  virtual std::string Name() const = 0;

  /// @brief version name
  virtual std::string Version() const = 0;

  /// @brief Plugin Type
  PluginType Type() const override { return PluginType::kSelector; }

  /// @brief Interface for getting the routing of a node of the called service
  /// @param[in] info Routing selection information
  /// @param[out] endpoint Routing selection result
  /// @return int Returns 0 on success, -1 on failure
  virtual int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) = 0;

  /// @brief Asynchronous interface for getting a called node
  /// @param[in] info Routing selection information
  /// @return Future<TrpcEndpointInfo> Returns a successful Future
  /// with ready state on success, or a failed Future with exception on failure
  virtual Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) = 0;

  /// @brief Interface for batch obtaining node routing information according to policy
  /// @param[in] info Routing selection information
  /// @param[out] endpoints Routing selection result
  /// @return int Returns 0 on success, -1 on failure
  virtual int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) = 0;

  /// @brief Asynchronous interface for batch obtaining node routing information according to policy
  /// @param[in] info Routing selection information
  /// @return Future<std::vector<TrpcEndpointInfo>> Returns a successful Future with ready state on
  /// success, or a failed Future with exception on failure
  virtual Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) = 0;

  /// @brief Interface for reporting invocation results
  /// @param result Invocation result
  /// @return int Returns 0 on success, -1 on failure
  virtual int ReportInvokeResult(const InvokeResult* result) = 0;

  /// @brief Interface for setting routing information for the called service
  /// @param info Service routing information
  /// @return int Returns 0 on success, -1 on failure
  virtual int SetEndpoints(const RouterInfo* info) = 0;

  /// @brief Interface for setting framework error code circuit breaker whitelist
  /// @param framework_retcodes Framework error code information
  /// @return bool Returns true on success, false on failure
  virtual bool SetCircuitBreakWhiteList(const std::vector<int>& framework_retcodes) { return false; }
};

using SelectorPtr = RefPtr<Selector>;

}  // namespace trpc
