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
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/loadbalance_naming_conf.h"
#include "trpc/naming/load_balance.h"

namespace trpc {
constexpr char kModuloHashLoadBalance[] = "trpc_modulohash_load_balance";

/// @brief consistent hash load balancing plugin
class ModuloHashLoadBalance : public LoadBalance {
 public:
  ModuloHashLoadBalance() = default;
  ~ModuloHashLoadBalance() = default;

  /// @brief Get the name of the load balancing plugin
  std::string Name() const override { return kModuloHashLoadBalance; }

  /// @brief Update the routing node information used by the load balancing
  int Update(const LoadBalanceInfo* info) override;

  /// @brief Return a callee node
  int Next(LoadBalanceResult& result) override;

 private:
  /// @brief Check if the load balancing information is different
  bool IsLoadBalanceInfoDiff(const LoadBalanceInfo* info);

  std::string GenerateKeysAsString(const SelectorInfo* info, std::vector<uint32_t> indexs);

  std::uint64_t Hash(const std::string& key, uint64_t num, const std::string& hash_func);

  struct InnerEndpointInfos {
    std::vector<TrpcEndpointInfo> endpoints;
    std::uint64_t hash;
  };

  std::unordered_map<std::string, ModuloHashLoadBalance::InnerEndpointInfos> callee_router_infos_;

  mutable std::shared_mutex mutex_;

  // std::unordered_map<std::string,
};

}  // namespace trpc
