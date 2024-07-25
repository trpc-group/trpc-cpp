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

#include "trpc/naming/common/util/loadbalance/hash/modulohash_load_balance.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include <algorithm>
#include <any>
#include <iostream>
#include <regex>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/common/util/hash/hash_func.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/naming/common/util/loadbalance/hash/common.h"


namespace trpc {

int ModuloHashLoadBalance::Init() noexcept{
  if (!trpc::TrpcConfig::GetInstance()->GetPluginConfig("loadbalance", kModuloHashLoadBalance, loadbalance_config_)) {
    TRPC_FMT_DEBUG("get loadbalance config failed, use default value");
  }

  bool res=CheckLoadBalanceSelectorConfig(loadbalance_config_);
  return res?0:-1;
}

bool ModuloHashLoadBalance::IsLoadBalanceInfoDiff(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    return false;
  }

  const SelectorInfo* select_info = info->info;
  std::shared_lock<std::shared_mutex> lock(mutex_);
  if (callee_router_infos_.end() == callee_router_infos_.find(select_info->name)) {
    return true;
  }

  return CheckLoadbalanceInfoDiff(callee_router_infos_[select_info->name], info->endpoints);
}


// Update the routing nodes used for load balancing
int ModuloHashLoadBalance::Update(const LoadBalanceInfo* info) {
  if (nullptr == info || nullptr == info->info || nullptr == info->endpoints) {
    TRPC_LOG_ERROR("Endpoint info of name is empty");
    return -1;
  }

  const SelectorInfo* select_info = info->info;

  if (IsLoadBalanceInfoDiff(info)) {

    std::vector<TrpcEndpointInfo> endpoints;
    endpoints.assign(info->endpoints->begin(), info->endpoints->end());

    std::unique_lock<std::shared_mutex> lock(mutex_);
    callee_router_infos_[select_info->name] = endpoints;
  }

  return 0;
}

int ModuloHashLoadBalance::Next(LoadBalanceResult& result) {
  if (nullptr == result.info) {
    return -1;
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = callee_router_infos_.find((result.info)->name);
  if (iter == callee_router_infos_.end()) {
    TRPC_LOG_ERROR("Router info of name " << (result.info)->name << " no found");
    return -1;
  }

  std::vector<TrpcEndpointInfo>& endpoints = iter->second;
  size_t endpoints_num = endpoints.size();
  if (endpoints_num < 1) {
    TRPC_LOG_ERROR("Router info of name is empty");
    return -1;
  }

  uint64_t hash = Hash(GenerateKeysAsString(result.info, loadbalance_config_.hash_args), loadbalance_config_.hash_func,
                       endpoints_num);

  result.result = endpoints[hash];

  return 0;
}

}  // namespace trpc