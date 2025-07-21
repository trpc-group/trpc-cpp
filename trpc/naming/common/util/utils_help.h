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

#include <stdint.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/naming/common/common_defs.h"

namespace trpc {

/// @brief Select multiple nodes
int SelectMultiple(const std::vector<TrpcEndpointInfo>& src, std::vector<TrpcEndpointInfo>* dst, const int select_size);

/// @brief Generate a unique ID for the IP:port pair of
/// the callee node for connection management performance optimization
class EndpointIdGenerator {
 public:
  /// @brief Get the unique ID of the node, the interface is not thread-safe
  /// @param endpoint The node, which is a string composed of the specific IP:port
  /// @return uint64_t The unique ID of the node
  /// @note Not thread-safe, external locking is required when used
  uint64_t GetEndpointId(const std::string& endpoint);

 private:
  std::unordered_map<std::string, uint64_t> endpoint_to_id_cache_;
  uint64_t next_id_ = 0;
};

}  // namespace trpc
