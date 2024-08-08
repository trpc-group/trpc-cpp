//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//

#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/load_balance_naming_conf.h"
#include "trpc/common/config/load_balance_naming_conf_parser.h"
#include "trpc/common/plugin.h"
#include "trpc/naming/common/util/utils_help.h"
#include "trpc/naming/load_balance.h"
#include "trpc/naming/selector.h"

namespace trpc {

/// @brief Polling Load Balance Test Class.
class TestPollingLoadBalance : public Selector {
 public:
  explicit TestPollingLoadBalance(const LoadBalancePtr& load_balance);

  /// @brief Returns the plugin name.
  std::string Name() const override { return "test_polling_load_balance"; }

  /// @brief Returns the plugin version.
  std::string Version() const override { return "0.0.1"; }

  /// @brief Initializes the plugin.
  /// @return Returns 0 on success, -1 on failure.
  int Init() noexcept override;

  /// @brief Selects a route information of a service node.
  /// @param info Selector information.
  /// @param endpoint Endpoint information.
  /// @return Returns 0 on success, -1 on failure.
  int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override;

  /// @brief Asynchronously selects a service node.
  /// @param info Selector information.
  /// @return Future of endpoint information.
  Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override;

  /// @brief Selects route information of nodes in batch according to the strategy.
  /// @param info Selector information.
  /// @param endpoints Vector to hold endpoint information.
  /// @return Returns 0 on success, -1 on failure.
  int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override;

  /// @brief Asynchronously selects nodes in batch according to the strategy.
  /// @param info Selector information.
  /// @return Future of vector of endpoint information.
  Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override;

  /// @brief Reports the result of an invocation.
  /// @param result Invocation result.
  /// @return Returns 0 on success, -1 on failure.
  int ReportInvokeResult(const InvokeResult* result) override;

  /// @brief Sets the routing information of the service.
  /// @param info Router information.
  /// @return Returns 0 on success, -1 on failure.
  int SetEndpoints(const RouterInfo* info) override;

  naming::LoadBalanceSelectorConfig loadbalance_config_;
  LoadBalance* GetLoadBalance(const std::string& name);

 private:
  struct EndpointsInfo {
    // Node information.
    std::vector<TrpcEndpointInfo> endpoints;
    // Node ID generator.
    EndpointIdGenerator id_generator;
  };

  std::unordered_map<std::string, EndpointsInfo> targets_map_;
  LoadBalancePtr default_load_balance_ = nullptr;
  std::shared_mutex mutex_;
};

using TestPollingLoadBalancePtr = RefPtr<TestPollingLoadBalance>;

}  // namespace trpc
