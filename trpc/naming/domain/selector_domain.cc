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

#include "trpc/naming/domain/selector_domain.h"

#include <memory>
#include <set>
#include <sstream>
#include <utility>

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/circuit_break/circuit_breaker_creator_factory.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/domain/domain_selector_conf_parser.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/util/domain_util.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string/string_util.h"
#include "trpc/util/time.h"

namespace trpc {

SelectorDomain::SelectorDomain(const LoadBalancePtr& load_balance) : default_load_balance_(load_balance) {
  TRPC_ASSERT(default_load_balance_);
}

int SelectorDomain::Init() noexcept {
  if (!trpc::TrpcConfig::GetInstance()->GetPluginConfig("selector", "domain", config_)) {
    TRPC_FMT_DEBUG("get selector domain config failed, use default value");
  }
  config_.Display();

  // domain update duration is 30s
  dn_update_interval_ = 30 * 1000;
  last_update_time_ = trpc::time::GetMilliSeconds();

  return 0;
}

int SelectorDomain::RefreshEndpointInfoByName(std::string dn_name, int dn_port,
                                              SelectorDomain::DomainEndpointInfo& endpointInfo) {
  std::vector<std::string> ip_list;
  trpc::util::GetAddrFromDomain(dn_name, ip_list);
  if (ip_list.empty()) {
    TRPC_LOG_ERROR("Get address info of " << dn_name << " failed");
    return -1;
  }

  // sort ip, Duplicate removal
  std::set<std::string> ip_list_set(ip_list.begin(), ip_list.end());

  endpointInfo.domain_name = dn_name;
  endpointInfo.port = dn_port;

  endpointInfo.endpoints.clear();
  std::vector<TrpcEndpointInfo> endpoints_exclude_ipv6;
  TrpcEndpointInfo endpoint;
  endpoint.port = dn_port;
  for (auto const& var : ip_list_set) {
    endpoint.host = var;
    endpoint.is_ipv6 = (endpoint.host.find(':') != std::string::npos);
    endpointInfo.endpoints.push_back(endpoint);

    if (!endpoint.is_ipv6) {
      endpoints_exclude_ipv6.push_back(endpoint);
    }
  }

  // exclude or not ipv6
  if (config_.exclude_ipv6 && endpoints_exclude_ipv6.size() != 0) {
    endpointInfo.endpoints.swap(endpoints_exclude_ipv6);
  }

  return 0;
}

// Used to update EndpointInof to targets_map and load_balance caches
int SelectorDomain::RefreshDomainInfo(const std::string& service_name,
                                      SelectorDomain::DomainEndpointInfo& dn_endpointInfo) {
  std::unordered_set<naming::CircuitBreakRecordKey, naming::CircuitBreakRecordKeyHash> keys;
  for (auto& endpoint : dn_endpointInfo.endpoints) {
    keys.emplace(naming::CircuitBreakRecordKey(endpoint.host, endpoint.port));
  }

  std::unique_lock<std::shared_mutex> uniq_lock(mutex_);
  // Generate a unique id for the node
  auto iter = targets_map_.find(service_name);
  if (iter != targets_map_.end()) {
    // If the service name is in the cache, use the original id generator
    dn_endpointInfo.id_generator = std::move(iter->second.id_generator);
    dn_endpointInfo.circuit_breaker = std::move(iter->second.circuit_breaker);
  } else {
    dn_endpointInfo.circuit_breaker = CreateCircuitBreaker(service_name);
  }

  // add or remove the status statistics in the circuit breaker based on the node set information
  if (dn_endpointInfo.circuit_breaker != nullptr) {
    dn_endpointInfo.circuit_breaker->Reserve(keys);
  }
  for (auto& item : dn_endpointInfo.endpoints) {
    std::string endpoint = item.host + ":" + std::to_string(item.port);
    item.id = dn_endpointInfo.id_generator.GetEndpointId(endpoint);
  }

  targets_map_[service_name] = dn_endpointInfo;

  uniq_lock.unlock();

  // update loadbalance cache
  DoUpdate(service_name, "");
  return 0;
}

LoadBalance* SelectorDomain::GetLoadBalance(const std::string& name) {
  if (!name.empty()) {
    auto load_balance = LoadBalanceFactory::GetInstance()->Get(name).get();
    if (load_balance) {
      return load_balance;
    }
  }

  return default_load_balance_.get();
}

naming::CircuitBreakerPtr SelectorDomain::CreateCircuitBreaker(const std::string& service_name) {
  if (config_.circuit_break_config.enable) {
    auto& plugin_name = config_.circuit_break_config.plugin_name;
    auto& plugin_config = config_.circuit_break_config.plugin_config;
    return naming::CircuitBreakerCreatorFactory::GetInstance()->Create(plugin_name, &plugin_config, service_name);
  }

  return nullptr;
}

bool SelectorDomain::DoUpdate(const std::string& service_name, const std::string& load_balance_name) {
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

bool SelectorDomain::CheckAndUpdate(const SelectorInfo* info) {
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
int SelectorDomain::Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) {
  if (nullptr == info || nullptr == endpoint) {
    TRPC_LOG_ERROR("Invalid parameter");
    return -1;
  }

  CheckAndUpdate(info);

  LoadBalanceResult load_balance_result;
  load_balance_result.info = info;
  auto lb = GetLoadBalance(info->load_balance_name);
  if (lb->Next(load_balance_result)) {
    TRPC_LOG_ERROR("Do load balance of " << info->name << " failed");
    return -1;
  }

  *endpoint = std::any_cast<TrpcEndpointInfo>(load_balance_result.result);
  return 0;
}

// Get a throttling interface asynchronously
Future<TrpcEndpointInfo> SelectorDomain::AsyncSelect(const SelectorInfo* info) {
  TrpcEndpointInfo endpoint;
  if (Select(info, &endpoint) == 0) {
    return MakeReadyFuture<TrpcEndpointInfo>(std::move(endpoint));
  }

  return MakeExceptionFuture<TrpcEndpointInfo>(CommonException("AsyncSelect failed"));
}

// An interface for fetching node routing information in bulk by policy
int SelectorDomain::SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) {
  if (nullptr == info || nullptr == endpoints) {
    TRPC_LOG_ERROR("Invalid parameter");
    return -1;
  }

  CheckAndUpdate(info);

  std::string callee = info->name;
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = targets_map_.find(callee);
  if (iter == targets_map_.end()) {
    TRPC_LOG_ERROR("router info of " << callee << " no found");
    return -1;
  }

  if (info->policy == SelectorPolicy::MULTIPLE) {
    if (!iter->second.available_endpoints.empty()) {
      SelectMultiple(iter->second.available_endpoints, endpoints, info->select_num);
    } else {
      SelectMultiple(iter->second.endpoints, endpoints, info->select_num);
    }
  } else {
    *endpoints = iter->second.endpoints;
  }

  return 0;
}

// Asynchronous interface to fetch nodes' routing information in bulk by policy
Future<std::vector<TrpcEndpointInfo>> SelectorDomain::AsyncSelectBatch(const SelectorInfo* info) {
  std::vector<TrpcEndpointInfo> endpoints;
  if (SelectBatch(info, &endpoints) == 0) {
    return MakeReadyFuture<std::vector<TrpcEndpointInfo>>(std::move(endpoints));
  }

  return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("AsyncSelectBatch fail"));
}

bool SelectorDomain::IsSuccess(int framework_result) {
  if (framework_result == TrpcRetCode::TRPC_INVOKE_SUCCESS) {
    return true;
  }

  return circuitbreak_whitelist_.Contains(framework_result);
}

// Call the result reporting interface
int SelectorDomain::ReportInvokeResult(const InvokeResult* result) {
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

int SelectorDomain::SetEndpoints(const RouterInfo* info) {
  if (nullptr == info) {
    TRPC_LOG_ERROR("Invalid parameter: router info is empty");
    return -1;
  }

  // Initialize hostname information, only support passing one hostname
  if (info->info.size() != 1) {
    TRPC_LOG_ERROR("Router info is invalid");
    return -1;
  }

  // Get the ip for the domain name
  std::string dn_name = info->info[0].host;
  int dn_port = info->info[0].port;
  SelectorDomain::DomainEndpointInfo endpointInfo;
  if (0 != RefreshEndpointInfoByName(dn_name, dn_port, endpointInfo)) {
    TRPC_LOG_ERROR("RefreshEndpointInfoByName of name" << dn_name << " failed");
    return -1;
  }

  RefreshDomainInfo(info->name, endpointInfo);
  return 0;
}

bool SelectorDomain::NeedUpdate() {
  uint64_t current_time = trpc::time::GetMilliSeconds();
  uint64_t next_refresh_time = last_update_time_ + dn_update_interval_;
  if (next_refresh_time < current_time) {
    last_update_time_ = current_time;
    return true;
  }

  return false;
}

// Return 0 on success, -1 on failure
int SelectorDomain::UpdateEndpointInfo() {
  // copy first
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto targets_map = targets_map_;
  lock.unlock();
  int targets_count = targets_map_.size();
  int success_count = 0;

  for (auto& item : targets_map) {
    // Update node information
    SelectorDomain::DomainEndpointInfo endpointInfo;
    if (!RefreshEndpointInfoByName(item.second.domain_name, item.second.port, endpointInfo)) {
      TRPC_LOG_DEBUG("Update endpointInfo of " << item.first << ":" << item.second.domain_name << " success");
      // Update node info to cache
      RefreshDomainInfo(item.first, endpointInfo);
      success_count++;
    }
  }

  return (targets_count == 0 || success_count > 0) ? 0 : -1;
}

void SelectorDomain::Start() noexcept {
  TRPC_LOG_DEBUG("Start domain selector task");
  if (task_id_ == 0) {
    task_id_ = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
        [this]() {
          if (NeedUpdate()) {
            UpdateEndpointInfo();
          }
          TRPC_LOG_TRACE("SelectorDomainTask Running");
        },
        200, "SelectorDomain");
  }
}

void SelectorDomain::Stop() noexcept {
  if (task_id_) {
    PeripheryTaskScheduler::GetInstance()->StopInnerTask(task_id_);
    task_id_ = 0;
  }
}

bool SelectorDomain::SetCircuitBreakWhiteList(const std::vector<int>& framework_retcodes) {
  circuitbreak_whitelist_.SetCircuitBreakWhiteList(framework_retcodes);
  return true;
}

}  // namespace trpc
