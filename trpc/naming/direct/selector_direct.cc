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

#include "trpc/naming/direct/selector_direct.h"

#include <memory>
#include <sstream>
#include <utility>

#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/common/util/loadbalance/weighted_round_robin/weighted_round_robin_load_balancer.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_util.h"

namespace trpc {

SelectorDirect::SelectorDirect(const LoadBalancePtr& load_balance) : default_load_balance_(load_balance) {
  TRPC_ASSERT(default_load_balance_);
}

LoadBalance* SelectorDirect::GetLoadBalance(const std::string& name) {
  if (!name.empty()) {
    auto load_balance = LoadBalanceFactory::GetInstance()->Get(name).get();
    if (load_balance) {
      return load_balance;
    }
  }

  return default_load_balance_.get();
}

// Get the routing interface of the node being called
int SelectorDirect::Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) {
  LoadBalanceResult load_balance_result;
  load_balance_result.info = info;
  auto lb = GetLoadBalance(info->load_balance_name);
  if (lb->Next(load_balance_result)) {
    std::string error_str = "Do load balance of " + info->name + " failed";
    TRPC_LOG_ERROR(error_str);
    return -1;
  }

  *endpoint = std::move(std::any_cast<TrpcEndpointInfo>(load_balance_result.result));
  return 0;
}

// Get a throttling interface asynchronously
Future<TrpcEndpointInfo> SelectorDirect::AsyncSelect(const SelectorInfo* info) {
  LoadBalanceResult load_balance_result;
  load_balance_result.info = info;
  auto lb = GetLoadBalance(info->load_balance_name);
  if (lb->Next(load_balance_result)) {
    std::string error_str = "Do load balance of " + info->name + " failed";
    TRPC_LOG_ERROR(error_str);
    return MakeExceptionFuture<TrpcEndpointInfo>(CommonException(error_str.c_str()));
  }

  TrpcEndpointInfo endpoint = std::move(std::any_cast<TrpcEndpointInfo>(load_balance_result.result));
  return MakeReadyFuture<TrpcEndpointInfo>(std::move(endpoint));
}

// An interface for fetching node routing information in bulk by policy
int SelectorDirect::SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) {
  if (nullptr == info || nullptr == endpoints) {
    TRPC_LOG_ERROR("Invalid parameter");
    return -1;
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto it = targets_map_.find(info->name);
  if (it == targets_map_.end()) {
    std::stringstream error_str;
    error_str << "router info of " << info->name << " no found";
    TRPC_LOG_ERROR(error_str.str());
    return -1;
  }

  if (info->policy == SelectorPolicy::MULTIPLE) {
    SelectMultiple(it->second.endpoints, endpoints, info->select_num);
  } else {
    *endpoints = it->second.endpoints;
  }
  return 0;
}

// Asynchronous interface to fetch nodes' routing information in bulk by policy
Future<std::vector<TrpcEndpointInfo>> SelectorDirect::AsyncSelectBatch(const SelectorInfo* info) {
  if (nullptr == info) {
    TRPC_LOG_ERROR("Invalid parameter: selectinfo is empty");
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("Selectorinfo is null"));
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto it = targets_map_.find(info->name);
  if (it == targets_map_.end()) {
    std::stringstream error_str;
    error_str << "router info of " << info->name << " no found";
    TRPC_LOG_ERROR(error_str.str());
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException(error_str.str().c_str()));
  }

  std::vector<TrpcEndpointInfo> endpoints;
  if (info->policy == SelectorPolicy::MULTIPLE) {
    SelectMultiple(it->second.endpoints, &endpoints, info->select_num);
  } else {
    endpoints = it->second.endpoints;
  }

  return MakeReadyFuture<std::vector<TrpcEndpointInfo>>(std::move(endpoints));
}

// Call the result reporting interface
int SelectorDirect::ReportInvokeResult(const InvokeResult* result) {
  if (nullptr == result) {
    TRPC_LOG_ERROR("Invalid parameter: invoke result is empty");
    return -1;
  }

  return 0;
}

int SelectorDirect::SetEndpoints(const RouterInfo* info) {
  if (nullptr == info) {
    TRPC_LOG_ERROR("Invalid parameter: router info is empty");
    return -1;
  }

  // Generate a unique id for each node, then put the node in the cache
  EndpointsInfo endpoints_info;
  endpoints_info.endpoints = info->info;

  std::unique_lock<std::shared_mutex> uniq_lock(mutex_);
  auto iter = targets_map_.find(info->name);
  if (iter != targets_map_.end()) {
    // If the service name is in the cache, use the original id generator
    endpoints_info.id_generator = std::move(iter->second.id_generator);
  }

  for (auto& item : endpoints_info.endpoints) {
    std::string endpoint = item.host + ":" + std::to_string(item.port);
    item.id = endpoints_info.id_generator.GetEndpointId(endpoint);
  }

  targets_map_[info->name] = endpoints_info;
  uniq_lock.unlock();

  // Update service routing information to default loadbalance
  SelectorInfo select_info;
  select_info.name = info->name;
  select_info.context = nullptr;
  LoadBalanceInfo load_balance_info;
  load_balance_info.info = &select_info;
  load_balance_info.endpoints = &endpoints_info.endpoints;
  default_load_balance_->Update(&load_balance_info);
  return 0;
}

}  // namespace trpc
