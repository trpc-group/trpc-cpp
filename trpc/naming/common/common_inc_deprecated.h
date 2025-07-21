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

#pragma once

#include <limits>
#include <map>
#include <string>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/util/align.h"
#include "trpc/util/object_pool/object_pool.h"

namespace trpc {

// Invocation result, is deprecated. Please use trpc::TrpcRetCode instead.
// The framework_result member of InvokeResult is now explicitly defined as a framework error code, so there is no need
// for CallRetStatus to do intermediate conversion. When businesses report on their own, they should use the framework
// error code directly instead of CallRetStatus here.
enum CallRetStatus {
  // Invocation succeeded
  kCallRetOk [[deprecated("Use trpc::TrpcRetCode instead")]] = TrpcRetCode::TRPC_INVOKE_SUCCESS,
  // Invocation timed out
  kCallRetTimeout [[deprecated("Use trpc::TrpcRetCode instead")]] = TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR,
  // Failure other than timeout
  kCallRetError [[deprecated("Use trpc::TrpcRetCode instead")]] = TrpcRetCode::TRPC_CLIENT_CONNECT_ERR
};

// Routing strategy
enum RouteFlag {
  // No strategy specified, default value
  kRouteNone = 0,
  // Route selection result includes unhealthy nodes
  kRouteIncludeUnhealthy = 1,
  // // Disable service router selection
  // kRouteDisableServiceRouter = 1 << 1,
};

/// Load balancing type used for naming service routing selection
enum class LoadBalanceType {
  // Default value, i.e., use the default strategy set by each selector plugin
  DEFAULT,
  // Weighted random
  WEIGHTED_RANDOM,
  // brpc locality_aware algorithm
  BRPC_LOCALITY_AWARE,
  // Consistent hash (using murmur3 algorithm)
  RING_HASH,
  // brpc cmurmurhash consistent hash algorithm
  BRPC_MURMUR_HASH,
  // Modulo hash
  MODULO_HASH,
};

// Convert string type load balancing type to LoadBalanceType, used for configuration files
inline LoadBalanceType GetLoadBalanceType(const std::string& input) {
  if (!input.compare("ringhash")) {
    return LoadBalanceType::RING_HASH;
  } else if (!input.compare("modulohash")) {
    return LoadBalanceType::MODULO_HASH;
  } else if (!input.compare("weightedrandom")) {
    return LoadBalanceType::WEIGHTED_RANDOM;
  } else if (!input.compare("brpcmurmurhash")) {
    return LoadBalanceType::BRPC_MURMUR_HASH;
  } else if (!input.compare("localityAware")) {
    return LoadBalanceType::BRPC_LOCALITY_AWARE;
  } else {
    return LoadBalanceType::DEFAULT;
  }
}

// Naming-related metadata types, is deprecated.
enum NamingMetadataType {
  // Polaris rule route label type
  kRuleRouteLable [[deprecated("Don't use it anymore")]],
  // Polaris metadata route label type
  kDstMetaRouteLable [[deprecated("Don't use it anymore")]],
  // Polaris circuit breaker label type
  kCircuitBreakLable [[deprecated("Don't use it anymore")]],
  // Polaris subset label used for do circuit break by set
  kSubsetLabel [[deprecated("Don't use it anymore")]],
  // Number of metadata type
  kTypeNum,
};

struct NamingExtendSelectInfo {
  std::string name_space;
  std::string callee_set_name;
  std::string canary_label;
  bool enable_set_force{false};
  bool disable_servicerouter{false};
  uint64_t locality_aware_info{0};
  uint32_t replicate_index{0};
  std::map<std::string, std::string> metadata[NamingMetadataType::kTypeNum];
  bool include_unhealthy{false};
};

namespace object_pool {

template <>
struct ObjectPoolTraits<NamingExtendSelectInfo> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

}  // namespace trpc
