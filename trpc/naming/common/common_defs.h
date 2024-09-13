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

#include <any>
#include <map>
#include <string>
#include <vector>

#include "trpc/client/client_context.h"
#include "trpc/naming/common/common_inc_deprecated.h"

namespace trpc {

/// @brief Structure of service registration information
struct RegistryInfo {
  /// Name of the route registered to the naming service
  std::string name;
  /// Host of the service listening on the naming service, supports IPv6 addresses
  std::string host;
  /// Port of the service listening on the naming service
  int port;
  /// Extended information field
  std::map<std::string, std::string> meta;
};

/// @brief Structure of service registration information for external use
struct TrpcRegistryInfo {
  /// Name of the naming service plugin
  std::string plugin_name;
  /// Service registration information
  RegistryInfo registry_info;
};

/// @brief Routing selection policy
enum class SelectorPolicy {
  /// Select one
  ONE,
  /// Select all
  ALL,
  /// Select according to set policy
  SET,
  /// Select according to proximity policy
  IDC,
  /// Select multiple
  MULTIPLE
};

/// @brief Structure of service discovery request
struct SelectorInfo {
  /// Name of the route used for service discovery
  std::string name;
  /// Selection policy
  SelectorPolicy policy = SelectorPolicy::ONE;
  /// Number of selections, used with MULTIPLE policy for backup request
  int select_num = 2;
  /// Name of the load balancing plugin used, default is empty
  /// (indicating the default load balancing of the selector plugin is used)
  std::string load_balance_name;

  /// is deprecated, please use load_balance_name instead
  /// Type of load balancing strategy used, used for multiple load balancing strategies within a load balancing plugin
  LoadBalanceType load_balance_type = LoadBalanceType::DEFAULT;

  /// Current user request context for service discovery
  ClientContextPtr context = nullptr;

  /// Specific routing selection information for each plugin (service level).
  const std::any* extend_select_info = nullptr;

  /// Indicates whether the request comes from the SelectorWorkFlow of the framework
  bool is_from_workflow = false;
};

/// @brief Structure of service discovery information for external use
struct TrpcSelectorInfo {
  /// Name of the naming service plugin
  std::string plugin_name;
  /// Service discovery information
  SelectorInfo selector_info;
};

/// Invalid endpoint ID
constexpr uint64_t kInvalidEndpointId = UINT64_MAX;

/// @brief Structure of service discovery result for external use
struct TrpcEndpointInfo {
  /// Address of the routing service
  std::string host;
  /// Port of the routing service
  int port{0};
  /// Health status
  int status{0};
  /// Whether the host is an IPv6 address
  bool is_ipv6{false};
  /// Weight of the routing node
  uint32_t weight;
  /// Unique ID corresponding to the node (ip:port), used for performance optimization of
  /// connection management within the framework, not needed for framework users (except for selector plugin developers)
  uint64_t id{kInvalidEndpointId};
  /// Extended information field
  std::map<std::string, std::string> meta;
};

/// @brief Service routing information
struct RouterInfo {
  /// Name of the called service
  std::string name;
  /// Routing information of the called service, which supports setting the ip:port method
  std::vector<TrpcEndpointInfo> info;
  /// Name of the load balancing plugin. An empty string means the default load balancer (trpc_polling_load_balance)
  /// will be used.
  std::string load_balance_name;
};

/// @brief Structure representing the service routing information for external use
struct TrpcRouterInfo {
  /// Name of the naming service plugin
  std::string plugin_name;
  /// Routing information of the called service
  RouterInfo router_info;
};

/// @brief Structure representing the service discovery invocation result for external use
struct InvokeResult {
  /// Name of the called service, for Polaris it is "service name
  /// (excluding namespace, namespace is passed in through context)"
  std::string name;
  /// Invocation result of framework layer, i.e., framework error code
  int framework_result;
  /// Invocation result of business interface, i.e., business error code
  int interface_result;
  /// Request processing time, in milliseconds
  uint64_t cost_time;
  /// Current user request context
  ClientContextPtr context;
};

/// @brief Structure of service discovery invocation result
struct TrpcInvokeResult {
  /// Name of naming plugin
  std::string plugin_name;
  /// Invocation result of business interface, 0 for success, 1 for failure
  InvokeResult invoke_result;
};

/// @brief Circuit breaker whitelist related to framework error codes
struct TrpcCircuitBreakWhiteList {
  /// Name of naming plugin
  std::string plugin_name;

  /// Full set of framework error codes
  std::vector<int> framework_retcodes;
};

/// @brief Structure of service rate limiting information
struct LimitInfo {
  /// Name of the called service that needs to be rate limited
  std::string name;
  /// Polaris namespace, required if using Polaris
  std::string name_space;
  /// Labels used to match rate limiting rules
  std::map<std::string, std::string> labels;
};

/// @brief Return codes of rate limiting interface
enum LimitRetCode {
  /// No rate limiting
  kLimitOK = 0,
  /// Error occurred during rate limiting execution or no rate limiting is needed
  kLimitError,
  /// Trigger rate limiting and reject the request
  kLimitReject
};

/// @brief Structure of rate limiting result, used for rate limiting reporting
struct LimitResult {
  /// Name of the route used for rate limiting
  std::string name;
  ///< Polaris namespace, required if using Polaris
  std::string name_space;
  /// Labels used to match rate limiting rules
  std::map<std::string, std::string> labels;
  /// Rate limiting result
  LimitRetCode limit_ret_code;
  /// Invocation result of framework layer, i.e., framework error code
  int framework_result;
  /// Invocation result of business interface, i.e., business error code
  int interface_result;
  /// Request processing time
  uint64_t cost_time;
};

/// @brief Structure of service rate limiting information in external interface
struct TrpcLimitInfo {
  /// Name of rate limiting plugin
  std::string plugin_name;
  /// Rate limiting information
  LimitInfo limit_info;
};

/// @brief Structure of rate limiting invocation result
struct TrpcLimitResult {
  /// Name of rate limiting plugin
  std::string plugin_name;
  /// Rate limiting result
  LimitResult limit_result;
};

}  // namespace trpc
