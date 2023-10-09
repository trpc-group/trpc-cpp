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

#include "trpc/util/buffer/memory_pool/disabled_memory_pool.h"

#include <cstdlib>
#include <new>

#include "trpc/util/buffer/memory_pool/common.h"
#include "trpc/util/likely.h"

namespace trpc::memory_pool::disabled {

detail::Block* Allocate() noexcept {
  // Obtaining the registered memory allocation function for memory allocation.
  AllocateMemFunc alloc_func = GetAllocateMemFunc();
  char* mem = static_cast<char*>(alloc_func(alignof(detail::Block), GetMemBlockSize()));
  if (TRPC_UNLIKELY(!mem)) {
    return nullptr;
  }

  detail::Block* block = new (mem) detail::Block{.ref_count = 1, .data = mem + sizeof(detail::Block)};

  return block;
}

void Deallocate(detail::Block* block) noexcept {
  // Obtaining the registered memory deallocation function for memory deallocation.
  DeallocateMemFunc dealloc_fun = GetDeallocateMemFunc();
  dealloc_fun(static_cast<void*>(block));
}

}  // namespace trpc::memory_pool::disabled
