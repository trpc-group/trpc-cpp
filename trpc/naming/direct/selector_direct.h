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

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/common/plugin.h"
#include "trpc/naming/common/util/circuit_break/circuit_break_whitelist.h"
#include "trpc/naming/common/util/circuit_break/circuit_breaker.h"
#include "trpc/naming/common/util/utils_help.h"
#include "trpc/naming/direct/direct_selector_conf.h"
#include "trpc/naming/load_balance.h"
#include "trpc/naming/selector.h"

namespace trpc {

/// @brief The SelectorDirect class is a plugin that implements the Selector interface for direct service discovery.
class SelectorDirect : public Selector {
 public:
  explicit SelectorDirect(const LoadBalancePtr& load_balance);

  /// @brief Initialization
  /// @return Returns 0 on success, -1 on failure
  int Init() noexcept override;

  /// @brief Return the name of the plugin.
  /// @return The name of the plugin.
  std::string Name() const override { return "direct"; }

  /// @brief Return the version of the plugin.
  /// @return The version of the plugin.
  std::string Version() const override { return "0.0.1"; }

  /// @brief Selects a single endpoint for the target service.
  /// @param info The selector information.
  /// @param endpoint The output parameter to store the selected endpoint.
  /// @return 0 on success, -1 on failure.
  int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override;

  /// @brief Asynchronously selects a single endpoint for the target service.
  /// @param info The selector information.
  /// @return A future that will contain the selected endpoint.
  Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override;

  /// @brief Selects multiple endpoints for the target service.
  /// @param info The selector information.
  /// @param endpoints The output parameter to store the selected endpoints.
  /// @return 0 on success, -1 on failure.
  int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override;

  /// @brief Asynchronously selects multiple endpoints for the target service.
  /// @param info The selector information.
  /// @return A future that will contain the selected endpoints.
  Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override;

  /// @brief Reports the result of an invocation to the plugin.
  /// @param result The invocation result.
  /// @return 0 on success, -1 on failure.
  int ReportInvokeResult(const InvokeResult* result) override;

  /// @brief Sets the endpoints for the target service.
  /// @param info The router information.
  /// @return 0 on success, -1 on failure.
  int SetEndpoints(const RouterInfo* info) override;

  /// @brief Sets the whitelist of framework error codes for circuit breaking reporting
  bool SetCircuitBreakWhiteList(const std::vector<int>& framework_retcodes) override;

 private:
  // Gets the load balancer plugin with the specified name.
  LoadBalance* GetLoadBalance(const std::string& name);

  naming::CircuitBreakerPtr CreateCircuitBreaker(const std::string& service_name);

  bool DoUpdate(const std::string& service_name, const std::string& load_balance_name);

  bool CheckAndUpdate(const SelectorInfo* info);

  bool IsSuccess(int framework_result);

 private:
  // The name of the default load balancer plugin.
  static const char default_load_balance_name_[];

  struct EndpointsInfo {
    // The endpoints for the target service.
    std::vector<TrpcEndpointInfo> endpoints;
    // The available endpoints for the target service.
    std::vector<TrpcEndpointInfo> available_endpoints;
    // The endpoint ID generator.
    EndpointIdGenerator id_generator;
    // circuit breaker
    naming::CircuitBreakerPtr circuit_breaker{nullptr};
  };

  std::unordered_map<std::string, EndpointsInfo> targets_map_;
  // The default load balancer plugin.
  LoadBalancePtr default_load_balance_;
  mutable std::shared_mutex mutex_;

  naming::DirectSelectorConfig config_;

  naming::CircuitBreakWhiteList circuitbreak_whitelist_;
};

using SelectorDirectPtr = RefPtr<SelectorDirect>;

}  // namespace trpc
