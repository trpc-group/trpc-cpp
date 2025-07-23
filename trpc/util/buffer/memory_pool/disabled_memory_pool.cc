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

#include "trpc/util/buffer/memory_pool/disabled_memory_pool.h"

#include <cstdlib>
#include <new>

#include "trpc/util/buffer/memory_pool/common.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::memory_pool::disabled {

detail::Block* Allocate() noexcept {
  // Obtaining the registered memory allocation function for memory allocation.
  AllocateMemFunc alloc_func = GetAllocateMemFunc();
  char* mem = static_cast<char*>(alloc_func(alignof(detail::Block), GetMemBlockSize()));
  if (TRPC_UNLIKELY(!mem)) {
    return nullptr;
  }

  detail::Block* block = new (mem) detail::Block{.ref_count = 1, .data = mem + sizeof(detail::Block)};

  GetStatistics().total_allocs_num.fetch_add(1, std::memory_order_relaxed);

  return block;
}

void Deallocate(detail::Block* block) noexcept {
  // Obtaining the registered memory deallocation function for memory deallocation.
  DeallocateMemFunc dealloc_fun = GetDeallocateMemFunc();
  dealloc_fun(static_cast<void*>(block));
  GetStatistics().total_frees_num.fetch_add(1, std::memory_order_relaxed);
}

Statistics& GetStatistics() noexcept {
  static Statistics statistics;
  return statistics;
}

void PrintStatistics() noexcept {
  Statistics& stat = GetStatistics();
  TRPC_FMT_INFO("disabled mem pool, total_allocs_num: {} ", stat.total_allocs_num.load(std::memory_order_relaxed));
  TRPC_FMT_INFO("disabled mem pool, total_frees_num: {} ", stat.total_frees_num.load(std::memory_order_relaxed));
}

}  // namespace trpc::memory_pool::disabled
