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

#include <atomic>
#include <memory>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/naming/load_balance.h"

namespace trpc {

constexpr char kPollingLoadBalance[] = "trpc_polling_load_balance";

/// @brief Round-robin load balancing plugin
class PollingLoadBalance : public LoadBalance {
 public:
  PollingLoadBalance() = default;

  ~PollingLoadBalance() = default;

  /// @brief Get the name of the load balancing plugin
  std::string Name() const override { return kPollingLoadBalance; }

  /// @brief Update the routing node information used by the load balancing
  int Update(const LoadBalanceInfo* info) override;

  /// @brief Return a callee node
  int Next(LoadBalanceResult& result) override;

 private:
  /// @brief Check if the load balancing information is different
  bool IsLoadBalanceInfoDiff(const LoadBalanceInfo* info);

  struct InnerEndpointInfos {
    std::vector<TrpcEndpointInfo> endpoints;
    std::uint32_t index;
  };

  std::unordered_map<std::string, PollingLoadBalance::InnerEndpointInfos> callee_router_infos_;
  mutable std::shared_mutex mutex_;
};

using PollingLoadBalancePtr = RefPtr<PollingLoadBalance>;

}  // namespace trpc
