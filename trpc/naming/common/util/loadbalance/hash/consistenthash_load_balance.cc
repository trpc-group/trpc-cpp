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

#include "trpc/naming/common/util/loadbalance/hash/consistenthash_load_balance.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/common/util/hash/hash_func.h"

#include <arpa/inet.h>
#include <bits/stdint-uintn.h>
#include <netdb.h>
#include <sys/socket.h>

#include <algorithm>
#include <any>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "trpc/naming/load_balance_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc {

bool ConsistentHashLoadBalance::IsLoadBalanceInfoDiff(const LoadBalanceInfo* info,
                                                      ConsistentHashLoadBalance::InnerEndpointInfos& endpoint_info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    return false;
  }

  const SelectorInfo* select_info = info->info;
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = callee_router_infos_.find(select_info->name);
  if (callee_router_infos_.end() == iter) {
    return true;
  }

  endpoint_info = iter->second;

  std::vector<TrpcEndpointInfo>& orig_endpoints = callee_router_infos_[select_info->name].endpoints;

  const std::vector<TrpcEndpointInfo>* new_endpoints = info->endpoints;
  if (orig_endpoints.size() != new_endpoints->size()) {
    return true;
  }

  int i = 0;
  for (auto& var : *new_endpoints) {
    auto orig_endpoint = orig_endpoints[i++];
    if (orig_endpoint.host != var.host || orig_endpoint.port != var.port) {
      return true;
    }

    if (orig_endpoint.status != var.status) {
      return true;
    }
  }

  return false;
}

std::string ConsistentHashLoadBalance::GenerateKeysAsString(const SelectorInfo* info, std::vector<uint32_t> indexs) {
  std::string key;
  for (int index : indexs) {
    switch (index) {
      case 0:
        key += info->name;
      case 1:
        key += std::to_string(static_cast<int>(info->policy));
      case 2:
        key += std::to_string(info->select_num);
      case 3:
        key += info->load_balance_name;
      case 4:
        key += std::to_string(info->is_from_workflow);
    }
  }
  return key;
}

// Update the routing nodes used for load balancing
int ConsistentHashLoadBalance::Update(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    TRPC_LOG_ERROR("Endpoint info of name is empty");
    return -1;
  }
  naming::LoadBalanceSelectorConfig loadbalance_config_;

  const SelectorInfo* select_info = info->info;

  const std::any* extend_select_info = select_info->extend_select_info;

  // if load balance config is exist,and init it
  if (extend_select_info != nullptr && extend_select_info->type() == typeid(naming::LoadBalanceSelectorConfig)) {
    loadbalance_config_ = std::any_cast<naming::LoadBalanceSelectorConfig>(*extend_select_info);
  }

  InnerEndpointInfos old_info;

  if (IsLoadBalanceInfoDiff(info, old_info)) {
    InnerEndpointInfos endpoint_info;

    endpoint_info.hash =
        Hash(GenerateKeysAsString(select_info, loadbalance_config_.hash_args), loadbalance_config_.hash_func);

    endpoint_info.hashring = old_info.hashring;

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

  auto info_iter = hashring.lower_bound(iter->second.hash);

  if (info_iter == hashring.end()) {
    info_iter = hashring.begin();
  }

  
  result.result = info_iter->second;
  // set hash to the next value
  iter->second.hash = info_iter->first + 1;

  return 0;
}

}  // namespace trpc