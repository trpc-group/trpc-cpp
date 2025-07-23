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

#include "trpc/naming/domain/selector_domain.h"

#include <memory>
#include <set>
#include <sstream>
#include <utility>

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
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
  dn_update_interval_ = 3600;
}

int SelectorDomain::Init() noexcept {
  if (!trpc::TrpcConfig::GetInstance()->GetPluginConfig("selector", "domain", select_config_)) {
    TRPC_FMT_DEBUG("get selector domain config failed, use default value");
  }

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
  if (select_config_.exclude_ipv6 && endpoints_exclude_ipv6.size() != 0) {
    endpointInfo.endpoints.swap(endpoints_exclude_ipv6);
  }

  return 0;
}

// Used to update EndpointInof to targets_map and load_balance caches
int SelectorDomain::RefreshDomainInfo(const SelectorInfo* info, SelectorDomain::DomainEndpointInfo& dn_endpointInfo) {
  if (nullptr == info) {
    TRPC_LOG_ERROR("Invalid parameter");
    return -1;
  }

  std::unique_lock<std::shared_mutex> uniq_lock(mutex_);
  // Generate a unique id for the node
  auto iter = targets_map_.find(info->name);
  if (iter != targets_map_.end()) {
    // If the service name is in the cache, use the original id generator
    dn_endpointInfo.id_generator = std::move(iter->second.id_generator);
  }

  for (auto& item : dn_endpointInfo.endpoints) {
    std::string endpoint = item.host + ":" + std::to_string(item.port);
    item.id = dn_endpointInfo.id_generator.GetEndpointId(endpoint);
  }

  targets_map_[info->name] = dn_endpointInfo;

  uniq_lock.unlock();

  // update loadbalance cache
  LoadBalanceInfo lb_info;
  lb_info.info = info;
  lb_info.endpoints = &dn_endpointInfo.endpoints;
  default_load_balance_->Update(&lb_info);
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

// Get the routing interface of the node being called
int SelectorDomain::Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) {
  if (nullptr == info || nullptr == endpoint) {
    TRPC_LOG_ERROR("Invalid parameter");
    return -1;
  }

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
  if (nullptr == info) {
    TRPC_LOG_ERROR("Selector info is null");
    return MakeExceptionFuture<TrpcEndpointInfo>(CommonException("Selector info is null"));
  }

  LoadBalanceResult load_balance_result;
  load_balance_result.info = info;
  auto lb = GetLoadBalance(info->load_balance_name);
  if (lb->Next(load_balance_result)) {
    std::string error_str = "Do load balance of " + info->name + " failed";
    TRPC_LOG_ERROR(error_str);
    return MakeExceptionFuture<TrpcEndpointInfo>(CommonException(error_str.c_str()));
  }

  TrpcEndpointInfo endpoint = std::any_cast<TrpcEndpointInfo>(load_balance_result.result);
  return MakeReadyFuture<TrpcEndpointInfo>(std::move(endpoint));
}

// An interface for fetching node routing information in bulk by policy
int SelectorDomain::SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) {
  if (nullptr == info || nullptr == endpoints) {
    TRPC_LOG_ERROR("Invalid parameter");
    return -1;
  }

  std::string callee = info->name;
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = targets_map_.find(callee);
  if (iter == targets_map_.end()) {
    TRPC_LOG_ERROR("router info of " << callee << " no found");
    return -1;
  }

  if (info->policy == SelectorPolicy::MULTIPLE) {
    SelectMultiple(iter->second.endpoints, endpoints, info->select_num);
  } else {
    *endpoints = iter->second.endpoints;
  }

  return 0;
}

// Asynchronous interface to fetch nodes' routing information in bulk by policy
Future<std::vector<TrpcEndpointInfo>> SelectorDomain::AsyncSelectBatch(const SelectorInfo* info) {
  if (nullptr == info) {
    TRPC_LOG_ERROR("Selector info is empty");
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("Selector info is empty"));
  }

  std::string callee = info->name;
  std::shared_lock<std::shared_mutex> lock(mutex_);
  auto iter = targets_map_.find(callee);
  if (iter == targets_map_.end()) {
    std::string error_str = "router info of " + callee + " no found";
    TRPC_LOG_ERROR(error_str);
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException(error_str.c_str()));
  }

  std::vector<TrpcEndpointInfo> endpoints;
  if (info->policy == SelectorPolicy::MULTIPLE) {
    SelectMultiple(iter->second.endpoints, &endpoints, info->select_num);
  } else {
    endpoints = iter->second.endpoints;
  }

  return MakeReadyFuture<std::vector<TrpcEndpointInfo>>(std::move(endpoints));
}

// Call the result reporting interface
int SelectorDomain::ReportInvokeResult(const InvokeResult* result) {
  if (nullptr == result) {
    TRPC_LOG_ERROR("Invalid parameter: invoke result is empty");
    return -1;
  }

  return 0;
}

int SelectorDomain::SetEndpoints(const RouterInfo* info) {
  if (nullptr == info) {
    TRPC_LOG_ERROR("Invalid parameter: router info is empty");
    return -1;
  }

  std::string callee_name = info->name;
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

  SelectorInfo selector_info;
  selector_info.name = info->name;
  RefreshDomainInfo(&selector_info, endpointInfo);
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

  for (auto item : targets_map) {
    // Update node information
    SelectorDomain::DomainEndpointInfo endpointInfo;
    if (!RefreshEndpointInfoByName(item.second.domain_name, item.second.port, endpointInfo)) {
      TRPC_LOG_DEBUG("Update endpointInfo of " << item.first << ":" << item.second.domain_name << " success");
      // Update node info to cache
      SelectorInfo selector_info;
      selector_info.name = item.first;
      RefreshDomainInfo(&selector_info, endpointInfo);
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

}  // namespace trpc
