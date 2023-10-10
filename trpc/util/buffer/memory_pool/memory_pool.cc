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

#include "trpc/util/buffer/memory_pool/memory_pool.h"

#include "trpc/util/buffer/memory_pool/common.h"

namespace trpc {

namespace memory_pool {

MemBlock* Allocate() {
#if defined(TRPC_DISABLED_MEM_POOL)
  return disabled::Allocate();
#elif defined(TRPC_SHARED_NOTHING_MEM_POOL)
  return shared_nothing::Allocate();
#else
  return global::Allocate();
#endif
}

void Deallocate(MemBlock* block) {
#if defined(TRPC_DISABLED_MEM_POOL)
  disabled::Deallocate(block);
#elif defined(TRPC_SHARED_NOTHING_MEM_POOL)
  shared_nothing::Deallocate(block);
#else
  global::Deallocate(block);
#endif
}

const MemStatistics& GetMemStatistics() noexcept {
#if defined(TRPC_DISABLED_MEM_POOL)
  return disabled::GetStatistics();
#elif defined(TRPC_SHARED_NOTHING_MEM_POOL)
  return shared_nothing::GetTlsStatistics();
#else
  return global::GetTlsStatistics();
#endif
}

void PrintMemStatistics() noexcept {
#if defined(TRPC_DISABLED_MEM_POOL)
  return disabled::PrintStatistics();
#elif defined(TRPC_SHARED_NOTHING_MEM_POOL)
  return shared_nothing::PrintTlsStatistics();
#else
  return global::PrintTlsStatistics();
#endif
}

}  // namespace memory_pool

std::size_t GetBlockMaxAvailableSize() { return memory_pool::GetMemBlockSize() - sizeof(memory_pool::MemBlock); }

RefPtr<memory_pool::MemBlock> MakeBlockRef(memory_pool::MemBlock* ptr) {
  return RefPtr<memory_pool::MemBlock>(adopt_ptr, ptr);
}

}  // namespace trpc
