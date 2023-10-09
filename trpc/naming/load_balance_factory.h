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

#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "trpc/naming/load_balance.h"

namespace trpc {

/// @brief Load balance plugin factory
class LoadBalanceFactory {
 public:
  /// @brief Get the singleton instance of LoadBalanceFactory
  static LoadBalanceFactory* GetInstance() {
    static LoadBalanceFactory instance;
    return &instance;
  }

  LoadBalanceFactory(const LoadBalanceFactory&) = delete;
  LoadBalanceFactory& operator=(const LoadBalanceFactory&) = delete;

  /// @brief Register a load balance plugin with the specified name
  bool Register(const LoadBalancePtr& load_balancer);

  /// @brief Get the load balance plugin with the specified name
  LoadBalancePtr Get(const std::string& name);

  /// @brief Get all loadbalance plugins
  const std::unordered_map<std::string, LoadBalancePtr>& GetAllPlugins();

  /// @brief Clear all registered load balance plugins
  void Clear();

 private:
  LoadBalanceFactory() = default;

 private:
  std::unordered_map<std::string, LoadBalancePtr> load_balances_;
};

}  // namespace trpc
