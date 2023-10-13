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

#include <cstddef>

#include <string>
#include <utility>

#include "trpc/util/buffer/memory_pool/common.h"
#include "trpc/util/buffer/memory_pool/disabled_memory_pool.h"
#include "trpc/util/buffer/memory_pool/global_memory_pool.h"
#include "trpc/util/buffer/memory_pool/shared_nothing_memory_pool.h"
#include "trpc/util/check.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

namespace memory_pool {

/// @brief Memory block type, an object that stores specific data.
/// @note In a shared-nothing architecture, it is not recommended to run in threads created by the business logic,
///       because when the business thread is released, if the memory blocks it contains continue to be used by other
///       threads, it can cause exceptions. If the business thread needs to use the memory blocks, it is necessary to
///       ensure that all memory blocks belonging to the business thread are released when it exits.
#if defined(TRPC_DISABLED_MEM_POOL)
using MemBlock = disabled::detail::Block;
using MemStatistics = disabled::Statistics;
#elif defined(TRPC_SHARED_NOTHING_MEM_POOL)
// Using shared nothing type
using MemBlock = shared_nothing::detail::Block;
using MemStatistics = shared_nothing::Statistics;
#else
// Using a global type.
using MemBlock = global::detail::Block;
using MemStatistics = global::Statistics;
#endif

/// @brief Allocate a MemBlock object to store data.
/// @return MemBlock pointer
MemBlock* Allocate();

/// @brief Freeing a memory block.
/// @param block MemBlock pointer
void Deallocate(MemBlock* block);

/// @brief Getting the statistics of the memory pool for the current thread.
/// @return Statistics
const MemStatistics& GetMemStatistics() noexcept;

/// @brief Print memory allocation/release information for the current thread's memory pool.
void PrintMemStatistics() noexcept;

}  // namespace memory_pool

/// @brief Wrap the MemBlock object in a Refer object for ease of use later.
/// @param ptr MemBlock pointer
RefPtr<memory_pool::MemBlock> MakeBlockRef(memory_pool::MemBlock* ptr);

/// @brief Get the available data area size in the Block.
/// @return Return the actual available size used to store data, which is the maximum amount of data that can be stored
///         in data.
std::size_t GetBlockMaxAvailableSize();

/// @private
template <>
struct RefTraits<memory_pool::MemBlock> {
  static void Reference(memory_pool::MemBlock* rc) { rc->ref_count.fetch_add(1, std::memory_order_relaxed); }
  static void Dereference(memory_pool::MemBlock* rc) {
    if (rc->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      TRPC_DCHECK_EQ(rc->ref_count.load(std::memory_order_relaxed), 0u);

      rc->ref_count.store(1, std::memory_order_relaxed);
      trpc::memory_pool::Deallocate(rc);
    }
  }
};

}  // namespace trpc
