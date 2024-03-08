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
#include <unordered_set>
#include <utility>

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/circuit_break/circuit_breaker_creator_factory.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/direct/direct_selector_conf_parser.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_util.h"
#include "trpc/util/time.h"

namespace trpc {

SelectorDirect::SelectorDirect(const LoadBalancePtr& load_balance) : default_load_balance_(load_balance) {
  TRPC_ASSERT(default_load_balance_);
}

int SelectorDirect::Init() noexcept {
  if (!TrpcConfig::GetInstance()->GetPluginConfig<naming::DirectSelectorConfig>("selector", "direct", config_)) {
    TRPC_FMT_DEBUG("Get plugin config fail, use default config");
  }

  config_.Display();
  return 0;
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

naming::CircuitBreakerPtr SelectorDirect::CreateCircuitBreaker(const std::string& service_name) {
  if (config_.circuit_break_config.enable) {
    auto& plugin_name = config_.circuit_break_config.plugin_name;
    auto& plugin_config = config_.circuit_break_config.plugin_config;
    return naming::CircuitBreakerCreatorFactory::GetInstance()->Create(plugin_name, &plugin_config, service_name);
  }

  return nullptr;
}

bool SelectorDirect::DoUpdate(const std::string& service_name, const std::string& load_balance_name) {
  std::unique_lock<std::shared_mutex> wlock(mutex_);
  auto it = targets_map_.find(service_name);
  if (it == targets_map_.end()) {
    return false;
  }
  auto& real_endpoints = it->second.endpoints;
  auto& available_endpoints = it->second.available_endpoints;

  auto& circuit_breaker = it->second.circuit_breaker;
  if (circuit_breaker != nullptr) {
    available_endpoints.clear();
    available_endpoints.reserve(real_endpoints.size());
    auto endpoints_status = circuit_breaker->GetAllStatus();
    for (auto& endpoint : real_endpoints) {
      naming::CircuitBreakRecordKey key(endpoint.host, endpoint.port);
      auto iter = endpoints_status.find(key);
      if (iter != endpoints_status.end()) {
        if (iter->second != naming::CircuitBreakStatus::kOpen) {
          available_endpoints.push_back(endpoint);
        }
      }
    }
  }

  // update route info
  SelectorInfo select_info;
  select_info.name = service_name;
  LoadBalanceInfo load_balance_info;
  load_balance_info.info = &select_info;
  // no available nodes, then select from all nodes
  if (!available_endpoints.empty()) {
    load_balance_info.endpoints = &available_endpoints;
  } else {
    load_balance_info.endpoints = &real_endpoints;
  }
  GetLoadBalance(load_balance_name)->Update(&load_balance_info);
  return true;
}

bool SelectorDirect::CheckAndUpdate(const SelectorInfo* info) {
  if (!config_.circuit_break_config.enable) {
    return false;
  }

  std::shared_lock<std::shared_mutex> rlock(mutex_);
  auto it = targets_map_.find(info->name);
  if (it == targets_map_.end()) {
    return false;
  }
  auto& circuit_breaker = it->second.circuit_breaker;
  if (circuit_breaker && circuit_breaker->StatusChanged(GetMilliSeconds())) {
    // If the node status changes, update the nodes set in the load balancer
    rlock.unlock();
    return DoUpdate(info->name, info->load_balance_name);
  }

  return false;
}

// Get the routing interface of the node being called
int SelectorDirect::Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) {
  CheckAndUpdate(info);

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
  TrpcEndpointInfo endpoint;
  if (Select(info, &endpoint) == 0) {
    return MakeReadyFuture<TrpcEndpointInfo>(std::move(endpoint));
  }

  std::string error_str = "Do load balance of " + info->name + " failed";
  return MakeExceptionFuture<TrpcEndpointInfo>(CommonException(error_str.c_str()));
}

// An interface for fetching node routing information in bulk by policy
int SelectorDirect::SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) {
  if (nullptr == info || nullptr == endpoints) {
    TRPC_LOG_ERROR("Invalid parameter");
    return -1;
  }

  CheckAndUpdate(info);

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto it = targets_map_.find(info->name);
  if (it == targets_map_.end()) {
    std::stringstream error_str;
    error_str << "router info of " << info->name << " no found";
    TRPC_LOG_ERROR(error_str.str());
    return -1;
  }

  if (info->policy == SelectorPolicy::MULTIPLE) {
    if (!it->second.available_endpoints.empty()) {
      SelectMultiple(it->second.available_endpoints, endpoints, info->select_num);
    } else {
      SelectMultiple(it->second.endpoints, endpoints, info->select_num);
    }
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

  std::vector<TrpcEndpointInfo> endpoints;
  if (SelectBatch(info, &endpoints) == 0) {
    return MakeReadyFuture<std::vector<TrpcEndpointInfo>>(std::move(endpoints));
  }

  std::string error_str = "router info of " + info->name + " no found";
  return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException(error_str.c_str()));
}

bool SelectorDirect::IsSuccess(int framework_result) {
  if (framework_result == TrpcRetCode::TRPC_INVOKE_SUCCESS) {
    return true;
  }

  return circuitbreak_whitelist_.Contains(framework_result);
}

// Call the result reporting interface
int SelectorDirect::ReportInvokeResult(const InvokeResult* result) {
  if (nullptr == result || !config_.circuit_break_config.enable) {
    return -1;
  }

  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto it = targets_map_.find(result->name);
  if (it != targets_map_.end()) {
    auto& circuit_breaker = it->second.circuit_breaker;
    if (circuit_breaker != nullptr) {
      naming::CircuitBreakRecordKey key(result->context->GetIp(), result->context->GetPort());
      bool success = IsSuccess(result->framework_result);
      circuit_breaker->AddRecordData(key, GetMilliSeconds(), success);
    }
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
  std::unordered_set<naming::CircuitBreakRecordKey, naming::CircuitBreakRecordKeyHash> keys;
  for (auto& endpoint : endpoints_info.endpoints) {
    keys.emplace(naming::CircuitBreakRecordKey(endpoint.host, endpoint.port));
  }

  std::unique_lock<std::shared_mutex> uniq_lock(mutex_);
  auto iter = targets_map_.find(info->name);
  if (iter != targets_map_.end()) {
    // If the service name is in the cache, use the original id generator
    endpoints_info.id_generator = std::move(iter->second.id_generator);
    endpoints_info.circuit_breaker = std::move(iter->second.circuit_breaker);
  } else {
    endpoints_info.circuit_breaker = CreateCircuitBreaker(info->name);
  }

  // add or remove the status statistics in the circuit breaker based on the node set information
  if (endpoints_info.circuit_breaker != nullptr) {
    endpoints_info.circuit_breaker->Reserve(keys);
  }

  for (auto& item : endpoints_info.endpoints) {
    std::string endpoint = item.host + ":" + std::to_string(item.port);
    item.id = endpoints_info.id_generator.GetEndpointId(endpoint);
  }

  targets_map_[info->name] = endpoints_info;
  uniq_lock.unlock();

  // Update service routing information to default loadbalance
  DoUpdate(info->name, "");
  return 0;
}

bool SelectorDirect::SetCircuitBreakWhiteList(const std::vector<int>& framework_retcodes) {
  circuitbreak_whitelist_.SetCircuitBreakWhiteList(framework_retcodes);
  return true;
}

}  // namespace trpc
