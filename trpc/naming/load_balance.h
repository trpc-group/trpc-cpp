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

#include <memory>
#include <string>
#include <vector>

#include "trpc/common/plugin.h"
#include "trpc/naming/common/common_defs.h"

namespace trpc {

struct LoadBalanceInfo {
  // Routing selection information
  const SelectorInfo* info;

  // Routing information for the called service
  const std::vector<TrpcEndpointInfo>* endpoints;
};

struct LoadBalanceResult {
  const SelectorInfo* info;
  std::any result;  // Used to output the routing selection result
};

/// @brief Load balancing base class
class LoadBalance : public Plugin {
 public:
  /// @brief Plugin Type
  PluginType Type() const override { return PluginType::kLoadbalance; }

  /// @brief Update the routing node information used by the load balancing algorithm
  /// @param info The routing selection information
  /// @return int 0: update succeeded
  ///             -1: update failed
  virtual int Update(const LoadBalanceInfo* info) = 0;

  /// @brief Get the next service endpoint to call
  /// @param result The result of the load balancing algorithm
  /// @return int 0: selection succeeded
  ///             -1: selection failed
  virtual int Next(LoadBalanceResult& result) = 0;
};

using LoadBalancePtr = RefPtr<LoadBalance>;

}  // namespace trpc
