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

#include "trpc/naming/common/util/loadbalance/hash/consistenthash_load_balance.h"

#include <arpa/inet.h>
#include <bits/stdint-uintn.h>
#include <netdb.h>
#include <sys/socket.h>

#include <algorithm>
#include <any>
#include <iostream>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/hash/hash_func.h"
#include "trpc/naming/common/util/loadbalance/hash/common.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int ConsistentHashLoadBalance::Init() noexcept {
  if (!trpc::TrpcConfig::GetInstance()->GetPluginConfig("loadbalance", kConsistentHashLoadBalance,
                                                        loadbalance_config_)) {
    TRPC_FMT_DEBUG("get loadbalance config failed, use default value");
  }

  bool res = CheckLoadBalanceSelectorConfig(loadbalance_config_);
  return res ? 0 : -1;
}

bool ConsistentHashLoadBalance::IsLoadBalanceInfoDiff(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    return false;
  }

  const SelectorInfo* select_info = info->info;
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = callee_router_infos_.find(select_info->name);
  if (callee_router_infos_.end() == iter) {
    return true;
  }

  return CheckLoadbalanceInfoDiff(callee_router_infos_[select_info->name].endpoints, info->endpoints);
}

// Update the routing nodes used for load balancing
int ConsistentHashLoadBalance::Update(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    TRPC_LOG_ERROR("Endpoint info of name is empty");
    return -1;
  }

  const SelectorInfo* select_info = info->info;

  InnerEndpointInfos old_info;

  if (IsLoadBalanceInfoDiff(info)) {
    {
      std::unique_lock<std::shared_mutex> lock(mutex_);
      if (callee_router_infos_.find(select_info->name) != callee_router_infos_.end()) {
        old_info = callee_router_infos_[select_info->name];
      }
    }
    InnerEndpointInfos endpoint_info;

    endpoint_info.hashring = std::move(old_info.hashring);
    endpoint_info.endpoints.assign(info->endpoints->begin(), info->endpoints->end());

    std::unordered_set<std::string> old_set;
    std::unordered_map<std::string, int> new_set;

    for (uint32_t i = 0; i < info->endpoints->size(); i++) {
      new_set[info->endpoints->at(i).host + std::to_string(info->endpoints->at(i).port)] = i;
    }

    for (const auto& endpoint : old_info.endpoints) {
      old_set.insert(endpoint.host + std::to_string(endpoint.port));
    }

    for (const auto& elem : new_set) {
      if (old_set.find(elem.first) == old_set.end()) {
        for (uint32_t i = 0; i < loadbalance_config_.hash_nodes; i++) {
          uint64_t key = Hash(elem.first + std::to_string(i), loadbalance_config_.hash_func);
          endpoint_info.hashring[key] = info->endpoints->at(elem.second);
        }
      }
    }
    for (const auto& elem : old_set) {
      if (new_set.find(elem) == new_set.end()) {
        for (uint32_t i = 0; i < loadbalance_config_.hash_nodes; i++) {
          uint64_t key = Hash(elem + std::to_string(i), loadbalance_config_.hash_func);
          endpoint_info.hashring.erase(key);
        }
      }
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    callee_router_infos_[select_info->name] = endpoint_info;
  }

  return 0;
}

int ConsistentHashLoadBalance::Next(LoadBalanceResult& result) {
  if (nullptr == result.info) {
    return -1;
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = callee_router_infos_.find((result.info)->name);
  if (iter == callee_router_infos_.end()) {
    TRPC_LOG_ERROR("Router info of name " << (result.info)->name << " no found");
    return -1;
  }

  std::map<std::uint64_t, TrpcEndpointInfo>& hashring = iter->second.hashring;
  size_t endpoints_num = hashring.size();
  if (endpoints_num < 1) {
    TRPC_LOG_ERROR("Router info of name is empty");
    return -1;
  }

  uint64_t hash = Hash(GenerateKeysAsString(result.info, loadbalance_config_.hash_args), loadbalance_config_.hash_func);

  auto info_iter = hashring.lower_bound(hash);

  if (info_iter == hashring.end()) {
    info_iter = hashring.begin();
  }

  result.result = info_iter->second;

  return 0;
}

}  // namespace trpc