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

#include "trpc/naming/common/util/utils_help.h"

#include <stdlib.h>

namespace trpc {

int SelectMultiple(const std::vector<TrpcEndpointInfo>& src, std::vector<TrpcEndpointInfo>* dst,
                   const int select_size) {
  int need_select_size = select_size;
  int instances_size = src.size();
  if (need_select_size > instances_size) {
    need_select_size = instances_size;
  }
  int pos = rand() % instances_size;
  if (pos + need_select_size > instances_size) {
    // Add instances_size-pos elements
    dst->assign(src.begin() + pos, src.end());
    // Add need_select_size - (instances_size-pos) elements
    int reserve_size = need_select_size - (instances_size - pos);
    dst->insert(dst->end(), src.begin(), src.begin() + reserve_size);
  } else {
    dst->assign(src.begin() + pos, src.begin() + pos + need_select_size);
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
