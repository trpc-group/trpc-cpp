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

#include "trpc/naming/pulling_test/selector_load_balance.h"

#include <memory>
#include <sstream>
#include <utility>

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/loadbalance/weighted_round_robin/weighted_round_robin_load_balancer.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_util.h"

namespace trpc {

TestPollingLoadBalance::TestPollingLoadBalance(const LoadBalancePtr& load_balance) {
  default_load_balance_ = load_balance;
  TRPC_ASSERT(default_load_balance_);
}

int TestPollingLoadBalance::Init() noexcept {
  if (!trpc::TrpcConfig::GetInstance()->GetPluginConfig("selector", "loadbalance", loadbalance_config_)) {
    TRPC_FMT_DEBUG("Failed to get selector domain config, using default value.");
  }

  return 0;
}

LoadBalance* TestPollingLoadBalance::GetLoadBalance(const std::string& name) {
  if (!name.empty()) {
    auto load_balance = LoadBalanceFactory::GetInstance()->Get(name).get();
    if (load_balance) {
      return load_balance;
    }
  }
  return default_load_balance_.get();
}

int TestPollingLoadBalance::Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) {
  LoadBalanceResult load_balance_result;
  load_balance_result.info = info;
  auto lb = GetLoadBalance(info->load_balance_name);
  if (lb->Next(load_balance_result)) {
    std::string error_str = "Failed to perform load balance for " + info->name;
    TRPC_LOG_ERROR(error_str);
    return -1;
  }

  *endpoint = std::move(std::any_cast<TrpcEndpointInfo>(load_balance_result.result));
  return 0;
}

Future<TrpcEndpointInfo> TestPollingLoadBalance::AsyncSelect(const SelectorInfo* info) {
  LoadBalanceResult load_balance_result;
  load_balance_result.info = info;
  auto lb = GetLoadBalance(info->load_balance_name);
  if (lb->Next(load_balance_result)) {
    std::string error_str = "Failed to perform load balance for " + info->name;
    TRPC_LOG_ERROR(error_str);
    return MakeExceptionFuture<TrpcEndpointInfo>(CommonException(error_str.c_str()));
  }

  TrpcEndpointInfo endpoint = std::move(std::any_cast<TrpcEndpointInfo>(load_balance_result.result));
  return MakeReadyFuture<TrpcEndpointInfo>(std::move(endpoint));
}

int TestPollingLoadBalance::SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) {
  if (info == nullptr || endpoints == nullptr) {
    TRPC_LOG_ERROR("Invalid parameter: info or endpoints is null.");
    return -1;
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto it = targets_map_.find(info->name);
  if (it == targets_map_.end()) {
    std::stringstream error_str;
    error_str << "Router info for " << info->name << " not found.";
    TRPC_LOG_ERROR(error_str.str());
    return -1;
  }

  if (info->policy == SelectorPolicy::MULTIPLE) {
    endpoints->resize(info->select_num);
    for (int i = 0; i < info->select_num; ++i) {
      Select(info, &(endpoints->at(i)));
    }
  } else {
    endpoints->resize(1);
    Select(info, &(endpoints->at(0)));
  }
  return 0;
}

Future<std::vector<TrpcEndpointInfo>> TestPollingLoadBalance::AsyncSelectBatch(const SelectorInfo* info) {
  if (info == nullptr) {
    TRPC_LOG_ERROR("Invalid parameter: selector info is null.");
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("Selector info is null."));
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto it = targets_map_.find(info->name);
  if (it == targets_map_.end()) {
    std::stringstream error_str;
    error_str << "Router info for " << info->name << " not found.";
    TRPC_LOG_ERROR(error_str.str());
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException(error_str.str().c_str()));
  }

  std::vector<TrpcEndpointInfo> endpoints;
  if (info->policy == SelectorPolicy::MULTIPLE) {
    endpoints.resize(info->select_num);
    for (int i = 0; i < info->select_num; ++i) {
      Select(info, &(endpoints.at(i)));
    }
  } else {
    endpoints.resize(1);
    Select(info, &(endpoints.at(0)));
  }

  return MakeReadyFuture<std::vector<TrpcEndpointInfo>>(std::move(endpoints));
}

int TestPollingLoadBalance::ReportInvokeResult(const InvokeResult* result) {
  if (result == nullptr) {
    TRPC_LOG_ERROR("Invalid parameter: invoke result is null.");
    return -1;
  }

  return 0;
}

int TestPollingLoadBalance::SetEndpoints(const RouterInfo* info) {
  if (info == nullptr) {
    TRPC_LOG_ERROR("Invalid parameter: router info is null.");
    return -1;
  }

  // Generate a unique ID for each node and then cache the node.
  EndpointsInfo endpoints_info;
  endpoints_info.endpoints = info->info;

  std::unique_lock<std::shared_mutex> unique_lock(mutex_);
  auto iter = targets_map_.find(info->name);
  if (iter != targets_map_.end()) {
    // If the service name is in the cache, use the original ID generator.
    endpoints_info.id_generator = std::move(iter->second.id_generator);
  }

  for (auto& item : endpoints_info.endpoints) {
    std::string endpoint = item.host + ":" + std::to_string(item.port);
    item.id = endpoints_info.id_generator.GetEndpointId(endpoint);
  }

  targets_map_[info->name] = endpoints_info;

  // Update service information to default load balance.
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
