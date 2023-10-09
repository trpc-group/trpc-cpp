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

#include "trpc/util/object_pool/shared_nothing.h"

namespace trpc::object_pool::shared_nothing {

namespace detail {

namespace {
std::atomic<uint32_t> cpu_id_gen{0};
}

uint32_t GetCpuId() {
  thread_local uint32_t tls_cpu_id = kInvalidCpuId;
  if (tls_cpu_id == kInvalidCpuId) {
    uint32_t cpu_id = cpu_id_gen.fetch_add(1, std::memory_order_relaxed);
    assert(cpu_id < kMaxCpus);

    tls_cpu_id = cpu_id;
  }

  return tls_cpu_id;
}

}  // namespace detail

}  // namespace trpc::object_pool::shared_nothing
