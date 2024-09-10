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
//

#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/load_balance_naming_conf.h"
#include "trpc/common/config/load_balance_naming_conf_parser.h"
#include "trpc/naming/load_balance.h"

namespace trpc {
constexpr char kSWRoundRobinLoadBalance[] = "trpc_swround_robin_loadbalance";

class SWRoundRobinLoadBalance : public LoadBalance {
 public:
  SWRoundRobinLoadBalance() = default;
  ~SWRoundRobinLoadBalance() override = default;

  std::string Name() const override { return kSWRoundRobinLoadBalance; }
  int Init() noexcept override;
  int Update(const LoadBalanceInfo* info) override;
  int Next(LoadBalanceResult& result) override;
  int SetEndpointInfoWeight(const std::string service_name, std::vector<TrpcEndpointInfo>& endpoints);

 private:
  bool IsLoadBalanceInfoDiff(const LoadBalanceInfo* info);

  struct InnerEndpointInfos {
    std::vector<int> weights;
    std::vector<TrpcEndpointInfo> endpoints;
    std::vector<int> current_weights;
    std::uint32_t total_weight;
  };
  naming::SWRoundrobinLoadBalanceConfig loadbalanceconfig;
  std::unordered_map<std::string, InnerEndpointInfos> callee_router_infos_;
  mutable std::shared_mutex mutex_;
};

using SWRoundRobinLoadBalancePtr = RefPtr<SWRoundRobinLoadBalance>;

}  // namespace trpc
