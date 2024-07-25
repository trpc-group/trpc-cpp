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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace trpc::naming {

/// @brief domain select plugin configuration
struct LoadBalanceSelectorConfig {
  /// @brief hash node number when consistent hash load balance algorithm is hash
  uint32_t hash_nodes{160};
  /// @brief hash content when load balance algorithm is hash,corresponding to SelectInfo
  /// 0：caller name 1: client ip 2：client port 3:info.name  4: callee name 5: info.loadbalance name
  std::vector<uint32_t> hash_args{0};
  /// @brief hash function when load balance algorithm is hash
  std::string hash_func{"murmur3"};

  /// @brief Print out the logger configuration.
  void Display() const;
};

}  // namespace trpc::naming