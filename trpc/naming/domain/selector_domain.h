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

#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/domain_naming_conf.h"
#include "trpc/common/config/domain_naming_conf_parser.h"
#include "trpc/common/plugin.h"
#include "trpc/naming/common/util/utils_help.h"
#include "trpc/naming/load_balance.h"
#include "trpc/naming/selector.h"

namespace trpc {

/// @brief Domain name service discovery plugin
class SelectorDomain : public Selector {
 public:
  explicit SelectorDomain(const LoadBalancePtr& load_balance);

  /// @brief Name of the plugin
  std::string Name() const override { return "domain"; }

  /// @brief Version of the plugin
  std::string Version() const override { return "0.0.2"; }

  /// @brief Initialization
  /// @return Returns 0 on success, -1 on failure
  int Init() noexcept override;

  /// @brief Interface used when the in-plugin implementation has threads to create, allowing unified usage
  void Start() noexcept override;

  /// @brief Interface for stopping threads created within the in-plugin implementation
  void Stop() noexcept override;

  /// @brief Interface for getting the routing of one node of the called service
  int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override;

  /// @brief Interface for asynchronously obtaining a called node
  Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override;

  /// @brief Interface for batch obtaining node routing information according to strategy
  int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override;

  /// @brief Asynchronous interface for batch obtaining node routing information according to strategy
  Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override;

  /// @brief Interface for reporting call results
  int ReportInvokeResult(const InvokeResult* result) override;

  /// @brief Interface for setting the routing information of the called service
  int SetEndpoints(const RouterInfo* info) override;

 private:
  struct DomainEndpointInfo {
    // Domain name of the called service
    std::string domain_name;
    // Port of the called service
    int port;
    // IP/port information of the called service
    std::vector<TrpcEndpointInfo> endpoints;
    // Node ID generator
    EndpointIdGenerator id_generator;
  };

  // Update the IP information corresponding to the domain name
  int RefreshEndpointInfoByName(std::string dn_name, int dn_port, SelectorDomain::DomainEndpointInfo& endpointInfo);

  // Update EndpointInfo to targets_map and load_balance cache
  int RefreshDomainInfo(const SelectorInfo* info, DomainEndpointInfo& dn_endpointInfo);

  // Determine whether to refresh routing information
  bool NeedUpdate();

  // Perform update operation
  int UpdateEndpointInfo();

  // Get the loadbalance plugin by name
  LoadBalance* GetLoadBalance(const std::string& name);

 private:
  // Default load balancer name
  static const char default_load_balance_name_[];

  std::unordered_map<std::string, DomainEndpointInfo> targets_map_;
  // Default load balancer
  LoadBalancePtr default_load_balance_;
  mutable std::shared_mutex mutex_;

  // Time interval for updating domain name information
  int dn_update_interval_;

  // Last update time
  uint64_t last_update_time_;

  // Business configuration items specified in the yaml file
  naming::DomainSelectorConfig select_config_;

  /// Task id of periodically updating node tasks
  uint64_t task_id_{0};
};

using SelectorDomainPtr = RefPtr<SelectorDomain>;

}  // namespace trpc
