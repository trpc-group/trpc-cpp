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

#include <atomic>
#include <memory>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/naming/load_balance.h"

namespace trpc {

constexpr char kWrrLoadBalance[] = "trpc_wrr_load_balance";

/// @brief Round-robin load balancing plugin
class WrrLoadBalance : public LoadBalance {
 public:
  WrrLoadBalance() = default;

  ~WrrLoadBalance() = default;

  /// @brief Get the name of the load balancing plugin
  std::string Name() const override { return kWrrLoadBalance; }

  /// @brief Update the routing node information used by the load balancing
  int Update(const LoadBalanceInfo* info) override;

  /// @brief Return a callee node
  int Next(LoadBalanceResult& result) override;

  /// @brief Set the weight of the callee node
  int SetWeight(const std::vector<int>& weights,SelectorInfo* select_info);
  
 private:
  /// @brief Check if the load balancing information is different
  bool IsLoadBalanceInfoDiff(const LoadBalanceInfo* info);

  struct InnerEndpointInfos {
    std::vector<TrpcEndpointInfo> endpoints;
    std::vector<int> weights;
    std::vector<int> current_weights;
    int total_weight;
  };

  std::unordered_map<std::string, WrrLoadBalance::InnerEndpointInfos> callee_router_infos_;
  mutable std::shared_mutex mutex_;
};

using WrrLoadBalancePtr = RefPtr<WrrLoadBalance>;

}  // namespace trpc
