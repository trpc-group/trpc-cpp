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

#include "trpc/common/config/loadbalance_naming_conf.h"
#include "trpc/common/config/loadbalance_naming_conf_parser.h"
#include "trpc/common/plugin.h"
#include "trpc/naming/common/util/utils_help.h"
#include "trpc/naming/load_balance.h"
#include "trpc/naming/selector.h"

namespace trpc::testing {

/// @brief The SelectorDirect class is a plugin that implements the Selector interface for direct service discovery.
class TestSelectorLoadBalance : public Selector {
 public:
  explicit TestSelectorLoadBalance(const LoadBalancePtr& load_balance);

  explicit TestSelectorLoadBalance()=default;

  /// @brief Return the name of the plugin.
  /// @return The name of the plugin.
  std::string Name() const override { return "test_loadbalance_selector"; }

  /// @brief Initialization
  /// @return Returns 0 on success, -1 on failure
  int Init() noexcept override;

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

  // set public to test
  naming::LoadBalanceSelectorConfig loadbalance_config_;

 private:
  /// @brief Gets the load balancer plugin with the specified name.
  /// @param name The name of the load balancer plugin.
  /// @return A pointer to the load balancer plugin.
  LoadBalance* GetLoadBalance(const std::string& name);

 private:
  // The name of the default load balancer plugin.
  static const char default_load_balance_name_[];

  struct EndpointsInfo {
    // The endpoints for the target service.
    std::vector<TrpcEndpointInfo> endpoints;
    // The endpoint ID generator.
    EndpointIdGenerator id_generator;
  };

  std::unordered_map<std::string, EndpointsInfo> targets_map_;
  // The default load balancer plugin.
  LoadBalancePtr default_load_balance_;
  mutable std::shared_mutex mutex_;
};

using TestSelectorLoadBalancePtr = RefPtr<TestSelectorLoadBalance>;

}  // namespace trpc::testing
