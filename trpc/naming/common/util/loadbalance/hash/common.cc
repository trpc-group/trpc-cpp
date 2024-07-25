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

#include "trpc/naming/common/util/loadbalance/hash/common.h"

#include <string>
#include <vector>

#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/common/util/hash/hash_func.h"

namespace trpc {
std::string GenerateKeysAsString(const SelectorInfo* info, std::vector<uint32_t>& indexs) {
  std::string key;
  for (int index : indexs) {
    switch (index) {
      case 0:
        if (info->context != nullptr) {
          key += info->context->GetCallerName();
        }
        break;
      case 1:
        if (info->context != nullptr) {
          key += info->context->GetIp();
        }
        break;
      case 2:
        if (info->context != nullptr) {
          key += std::to_string(info->context->GetPort());
        }
        break;
      case 3:
        key += info->name;
        break;

      case 4:
        if (info->context != nullptr) {
          key += info->context->GetCalleeName();
        }
        break;
      case 5:
        key += info->load_balance_name;
        break;
      default:
        break;
    }
  }
  return key;
}

bool CheckLoadBalanceSelectorConfig(naming::LoadBalanceSelectorConfig& loadbalance_config_) {
  bool res = true;
  for (int index : loadbalance_config_.hash_args) {
    if (index < 0 || index > HASH_NODES_MAX_INDEX) {
      TRPC_FMT_DEBUG("index in yaml configuration out of range, use default config");
      res = false;
      break;
    }
  }
  if (!res) {
    // set to default value
    loadbalance_config_.hash_args.assign({0});
  }
  if (HashFuncTable.find(loadbalance_config_.hash_func) == HashFuncTable.end()) {
    res = false;
    TRPC_FMT_DEBUG("hash func name is invalid, use default config");
    // set to default value
    loadbalance_config_.hash_func = MURMUR3;
  }
  if (loadbalance_config_.hash_nodes < 1) {
    res = false;
    TRPC_FMT_DEBUG("hash nodes is invalid, use default config");
    // set to default value
    loadbalance_config_.hash_nodes = 20;
  }

  return res;
}

bool CheckLoadbalanceInfoDiff(const std::vector<TrpcEndpointInfo>& orig_endpoints,
                              const std::vector<TrpcEndpointInfo>* new_endpoints) {
  if (orig_endpoints.size() != new_endpoints->size()) {
    return true;
  }

  int i = 0;
  for (auto& var : *new_endpoints) {
    auto orig_endpoint = orig_endpoints[i++];
    if (orig_endpoint.host != var.host || orig_endpoint.port != var.port) {
      return true;
    }

    if (orig_endpoint.status != var.status) {
      return true;
    }
  }

  return false;
}

}  // namespace trpc