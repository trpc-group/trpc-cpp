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

#include "trpc/naming/common/util/loadbalance/wrr/wrr_load_balance.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include <algorithm>
#include <any>
#include <iostream>
#include <regex>
#include <vector>

#include "trpc/naming/load_balance_factory.h"
#include "trpc/util/log/logging.h"
#include "wrr_load_balance.h"

namespace trpc {

bool WrrLoadBalance::IsLoadBalanceInfoDiff(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    return false;
  }

  const SelectorInfo* select_info = info->info;
  std::shared_lock<std::shared_mutex> lock(mutex_);
  if (callee_router_infos_.end() == callee_router_infos_.find(select_info->name)) {
    return true;
  }

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

// Update the routing nodes used for load balancing
int WrrLoadBalance::Update(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    TRPC_LOG_ERROR("Endpoint info of name is empty");
    return -1;
  }

  const SelectorInfo* select_info = info->info;
  if (IsLoadBalanceInfoDiff(info)) {
    InnerEndpointInfos endpoint_info;
    endpoint_info.endpoints.assign(info->endpoints->begin(), info->endpoints->end());

    std::unique_lock<std::shared_mutex> lock(mutex_);
    callee_router_infos_[select_info->name] = endpoint_info;
  }

  return 0;
}

// Used to return a regulated point
int WrrLoadBalance::Next(LoadBalanceResult& result) {
  if (nullptr == result.info) {
    return -1;
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = callee_router_infos_.find((result.info)->name);
  if (iter == callee_router_infos_.end()) {
    TRPC_LOG_ERROR("Router info of name " << (result.info)->name << " no found");
    return -1;
  }

  std::vector<TrpcEndpointInfo>& endpoints = iter->second.endpoints;
  size_t endpoints_num = endpoints.size();
  if (endpoints_num < 1) {
    TRPC_LOG_ERROR("Router info of name is empty");
    return -1;
}

  InnerEndpointInfos& endpoint_info = iter->second;
  // 开始时计算全部节点的 currentWeight = currentWeight + weight
  for (size_t i = 0; i < endpoints_num; i++) {
    endpoint_info.current_weights[i] += endpoint_info.weights[i];
  }
  // 选中 currentWeight 最大的节点，并设置 currentWeight = currentWeight - totalWeight
  int total_weight = endpoint_info.total_weight;
  int max_weight = 0;
  int index = 0;
  for (size_t i = 0; i < endpoints_num; i++) {
    int weight = endpoint_info.current_weights[i];
    if (weight > max_weight) {
      max_weight = weight;
      index = i;
    }
  }
  endpoint_info.current_weights[index] -= total_weight;

  result.result = endpoints[index];
  return 0;
}

// Set the weight of the callee node
int WrrLoadBalance::SetWeight(const std::vector<int>& weights, SelectorInfo* select_info)
{
  if (weights.empty()) {
    TRPC_LOG_ERROR("weights is empty");
    return -1;
  }
  // 从路由信息中查找
  auto iter = callee_router_infos_.find((select_info)->name);
  if (iter == callee_router_infos_.end()) {
    TRPC_LOG_ERROR("Router info of name " << (select_info)->name << " no found");
    return -1;
  }
  // 更新权重，计算全部节点的 currentWeight = currentWeight(0,0,...) + weight，计算总权重
  iter->second.weights = weights;
  iter->second.current_weights = weights;
  iter->second.total_weight = std::accumulate(weights.begin(), weights.end(), 0);
  return 0;
}
}  // namespace trpc
