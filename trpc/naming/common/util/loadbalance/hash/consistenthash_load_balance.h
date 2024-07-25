//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
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

#include "trpc/common/config/loadbalance_naming_conf.h"
#include "trpc/common/config/loadbalance_naming_conf_parser.h"
#include "trpc/naming/load_balance.h"

namespace trpc {
constexpr char kConsistentHashLoadBalance[] = "trpc_consistenthash_load_balance";

/// @brief consistent hash load balancing plugin
class ConsistentHashLoadBalance : public LoadBalance {
 public:
  ConsistentHashLoadBalance() = default;
  ~ConsistentHashLoadBalance() = default;

  /// @brief Get the name of the load balancing plugin
  std::string Name() const override { return kConsistentHashLoadBalance; }

  /// @brief Initialization
  /// @return Returns 0 on success, -1 on failure
  int Init() noexcept override;

  /// @brief Update the routing node information used by the load balancing
  int Update(const LoadBalanceInfo* info) override;

  /// @brief Return a callee node
  int Next(LoadBalanceResult& result) override;

 private:
  /// @brief Check if the load balancing information is different

  bool IsLoadBalanceInfoDiff(const LoadBalanceInfo* info);

  struct InnerEndpointInfos {
    std::vector<TrpcEndpointInfo> endpoints;
    std::map<std::uint64_t, TrpcEndpointInfo> hashring;
  };

  std::unordered_map<std::string, ConsistentHashLoadBalance::InnerEndpointInfos> callee_router_infos_;

  naming::LoadBalanceSelectorConfig loadbalance_config_;

  mutable std::shared_mutex mutex_;

};

}  // namespace trpc
