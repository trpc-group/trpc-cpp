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

#include "trpc/naming/common/util/utils_help.h"

#include <stdlib.h>

#include "trpc/util/algorithm/random.h"

namespace trpc {

int SelectMultiple(const std::vector<TrpcEndpointInfo>& src, std::vector<TrpcEndpointInfo>* dst,
                   const int select_size) {
  if (src.empty()) {
    return -1;
  }

  int need_select_size = select_size;
  int instances_size = src.size();
  if (need_select_size > instances_size) {
    need_select_size = instances_size;
  }
  dst->reserve(need_select_size);

  int pos = trpc::Random<uint32_t>() % instances_size;
  dst->push_back(src[pos]);
  if (instances_size == 1) {
    return 0;
  }

  // Randomly obtain the offset of the next node to be retrieved and retrieve 'need_select_size - 1' nodes
  // continuously (skipping the selected node).
  int pos_offset = trpc::Random<uint32_t>() % (instances_size - 1) + 1;
  int next_pos = pos + pos_offset;
  for (int i = 1; i < need_select_size; i++) {
    if (next_pos >= instances_size) {
      next_pos -= instances_size;
    }
    if (next_pos == pos) {
      next_pos++;
    }
    if (next_pos >= instances_size) {
      next_pos -= instances_size;
    }
    dst->push_back(src[next_pos++]);
  }

  return 0;
}

uint64_t EndpointIdGenerator::GetEndpointId(const std::string& endpoint) {
  auto iter = endpoint_to_id_cache_.find(endpoint);
  if (iter != endpoint_to_id_cache_.end()) {
    return iter->second;
  }

  uint64_t id = next_id_++;
  endpoint_to_id_cache_[endpoint] = id;
  return id;
}

}  // namespace trpc
