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

namespace trpc::memory_pool::global {

namespace detail {

/// @brief Memory blocks that store data in a memory pool.
struct alignas(64) Block {
  Block* next{nullptr};  ///< Pointer to the next Block object, used for allocation and deallocation, for easy access.
  std::atomic<std::uint32_t> ref_count{1};  ///< Reference count, used for smart pointer usage.
  bool need_free_to_system{true};           ///< Whether need to free memory to the system
  char* data{nullptr};                      ///< Data memory address, the actual address used to store business data.
};

}  // namespace detail

/// @brief Data statistics for the creation and release of global memory pool
struct alignas(64) Statistics {
  size_t total_allocs_num{0};           ///< Total number of Block objects allocated by the current thread.
  size_t total_frees_num{0};            ///< Number of times the tls releases Block objects.
  size_t allocs_from_tls_free_list{0};  ///< Number of times the tls gets Block objects from the free list.
  size_t frees_to_tls_free_list{0};     ///< Number of times the tls releases Block objects to the free block list.
  size_t allocs_from_tls_blocks{0};     ///< Number of times the tls gets Block objects from the block list.
  size_t allocs_from_system{0};         ///< Number of times the tls directly allocates Block objects from the system.
  size_t frees_to_system{0};            ///< Number of times the tls releases Block objects to the system.
};

/// @brief Allocate a Block object for storing data.
/// @return Block pointer.
detail::Block* Allocate() noexcept;

/// @brief Freeing a memory block.
/// @param block Block pointer.
void Deallocate(detail::Block* block) noexcept;

/// @brief Retrieve memory allocation/release information for the current thread's memory pool.
/// @return `Statistics` object
const Statistics& GetTlsStatistics();

/// @brief Print memory allocation/release information for the current thread's memory pool.
void PrintTlsStatistics();

/// @brief Pre-allocate a certain number of Block objects.
/// @param block_num Pre-allocate the number of Block objects.
/// @return int type，the number of Block objects successfully pre-allocated.
int PrewarmMemPool(uint32_t block_num = 0);

}  // namespace trpc::memory_pool::global
