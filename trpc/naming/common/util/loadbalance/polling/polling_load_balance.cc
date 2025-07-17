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

#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"

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

namespace trpc {

bool PollingLoadBalance::IsLoadBalanceInfoDiff(const LoadBalanceInfo* info) {
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
int PollingLoadBalance::Update(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    TRPC_LOG_ERROR("Endpoint info of name is empty");
    return -1;
  }

  const SelectorInfo* select_info = info->info;
  if (IsLoadBalanceInfoDiff(info)) {
    InnerEndpointInfos endpoint_info;
    endpoint_info.index = 0;
    endpoint_info.endpoints.assign(info->endpoints->begin(), info->endpoints->end());

    std::unique_lock<std::shared_mutex> lock(mutex_);
    callee_router_infos_[select_info->name] = endpoint_info;
  }

  return 0;
}

// Used to return a regulated point
int PollingLoadBalance::Next(LoadBalanceResult& result) {
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

  uint32_t index = 0;
  uint32_t id = __sync_fetch_and_add(&iter->second.index, 1);
  index = id % endpoints_num;

  result.result = endpoints[index];
  return 0;
}

}  // namespace trpc
