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

#include "trpc/naming/common/util/loadbalance/hash/selector_loadbalance_hash.h"

#include <memory>
#include <sstream>
#include <utility>

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_util.h"

namespace trpc::testing {


TestSelectorLoadBalance::TestSelectorLoadBalance(const LoadBalancePtr& load_balance)
    : default_load_balance_(load_balance) {
  TRPC_ASSERT(default_load_balance_);
}

int TestSelectorLoadBalance::Init() noexcept {
  if (!trpc::TrpcConfig::GetInstance()->GetPluginConfig("selector", "loadbalance", loadbalance_config_)) {
    TRPC_FMT_DEBUG("get selector domain config failed, use default value");
  }

  default_load_balance_ = LoadBalanceFactory::GetInstance()->Get(loadbalance_config_.load_balance_name);
  std::cout<<"the selector use loadbalance is "<<default_load_balance_->Name()<<std::endl;
  return 0;
}
LoadBalance* TestSelectorLoadBalance::GetLoadBalance(const std::string& name) {
  if (!name.empty()) {
    auto load_balance = LoadBalanceFactory::GetInstance()->Get(name).get();
    if (load_balance) {
      return load_balance;
    }
  }

  return default_load_balance_.get();
}

// Get the routing interface of the node being called
int TestSelectorLoadBalance::Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) {
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
Future<TrpcEndpointInfo> TestSelectorLoadBalance::AsyncSelect(const SelectorInfo* info) {
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
int TestSelectorLoadBalance::SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) {
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
Future<std::vector<TrpcEndpointInfo>> TestSelectorLoadBalance::AsyncSelectBatch(const SelectorInfo* info) {
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
int TestSelectorLoadBalance::ReportInvokeResult(const InvokeResult* result) {
  if (nullptr == result) {
    TRPC_LOG_ERROR("Invalid parameter: invoke result is empty");
    return -1;
  }

  return 0;
}

int TestSelectorLoadBalance::SetEndpoints(const RouterInfo* info) {
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
  std::any conf = loadbalance_config_;
  select_info.extend_select_info = &conf;
  LoadBalanceInfo load_balance_info;
  load_balance_info.info = &select_info;
  load_balance_info.endpoints = &endpoints_info.endpoints;
  default_load_balance_->Update(&load_balance_info);
  return 0;
}


}  // namespace trpc
