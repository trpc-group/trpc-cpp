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

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace trpc::fiber::detail {

/// @brief Set whether to use mmap to allocate memory stacks for fibers
void SetFiberPoolCreateWay(bool use_mmap);

/// @brief Set the stack size of memory blocks used by fibers
void SetFiberStackSize(uint32_t stack_size);

/// @brief Get the stack size of memory blocks used by fibers
uint32_t GetFiberStackSize();

/// @brief Set whether memory page protection is applied to memory blocks allocated through mmap
void SetFiberStackEnableGuardPage(bool flag);

/// @brief Set the number of pooled memory stacks allocated through mmap used by fibers
void SetFiberPoolNumByMmap(uint32_t fiber_pool_num_by_mmap);

/// @brief Enable debug fiber using gdb
void SetEnableGdbDebug(bool flag);

/// @brief Pre-allocate a certain number of fiber memory stacks
/// @return The number of successfully warmed up fiber memory blocks
int PrewarmFiberPool(uint32_t fiber_num);

/// @brief Allocate memory blocks stacks for fiber
/// @param[out] stack_ptr The address of the allocated memory block
/// @param[out] is_system Whether the allocated memory block is directly allocated from the system
/// @return Return true on success, false otherwise
bool Allocate(void** stack_ptr, bool* is_system) noexcept;

/// @brief Release memory block stacks used by fibers
/// @param stack_ptr The address of the allocated memory block
/// @param is_system Whether the allocated memory block is directly allocated from the system
void Deallocate(void* stack_ptr, bool is_system) noexcept;

// Diagnosing using memory analysis and detection tools
void ReuseStackInSanitizer(void* stack_ptr);

/// @brief Data statistics for creating/releasing memory pool for fiber stack
struct alignas(64) Statistics {
  // The total allocation count of memory stack for current thread's fiber
  size_t total_allocs_num = 0;
  // The number of memory stacks for fiber that the current thread obtains from the local TLS free list
  size_t allocs_from_tls_free_list = 0;
  // The number of available memory stacks for fiber that the current thread obtains from the global free list
  size_t pop_freelist_from_global = 0;
  // The number of memory stacks for fiber that the current thread obtains from the local TLS block
  size_t allocs_from_tls_block = 0;
  // The number of memory stacks for fiber that the current thread obtains from the global block
  size_t allocs_from_global_block = 0;
  // The number of memory stacks for fiber that the current thread directly allocates from the system
  size_t allocs_from_system = 0;
  // The total number of memory stacks for fiber that the current thread releases
  size_t total_frees_num = 0;
  // The number of memory stacks for fiber that the current thread releases to the local TLS free list
  size_t frees_to_tls_free_list = 0;
  // The number of times the idle list is recycled from the current thread's local pool to the global pool
  size_t push_freelist_to_global = 0;  //
  // The number of memory stacks for fiber that the current thread directly releases to the system
  size_t frees_to_system = 0;
};

/// @brief Get current thread's data statistics for creating/releasing memory pool for fiber stack
Statistics& GetTlsStatistics();

/// @brief Print current thread's data statistics for creating/releasing memory pool for fiber stack
void PrintTlsStatistics();

}  // namespace trpc::fiber::detail
