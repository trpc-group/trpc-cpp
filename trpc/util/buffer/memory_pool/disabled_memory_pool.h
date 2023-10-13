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
#include <cstddef>
#include <cstdint>

namespace trpc::memory_pool::disabled {

/// @private
namespace detail {

/// @brief Memory blocks that store data in the memory pool.
struct alignas(64) Block {
  std::atomic<std::uint32_t> ref_count{1};  ///< Reference counting, used for smart pointer implementations.
  char* data{nullptr};  ///< The memory address of the data, the actual address used to store business data.
};

}  // namespace detail

/// @brief Data statistics for the creation and release of the disabled memory pool.
/// @private For internal use purpose only.
struct alignas(64) Statistics {
  std::atomic<std::uint32_t> total_allocs_num{0};        ///< Total number of Block objects allocated.
  std::atomic<std::uint32_t> total_frees_num{0};         ///< Number of times the tls releases Block objects.
};

/// @brief Allocate a Block object for storing data
/// @return Block pointer
/// @private For internal use purpose only.
detail::Block* Allocate() noexcept;

/// @brief Freeing a memory block.
/// @param block Block pointer
/// @private For internal use purpose only.
void Deallocate(detail::Block* block) noexcept;


/// @brief Getting the statistics of the memory.
/// @return Statistics
/// @private For internal use purpose only.
Statistics& GetStatistics() noexcept;

/// @brief Print memory allocation/release information.
/// @private For internal use purpose only.
void PrintStatistics() noexcept;

}  // namespace trpc::memory_pool::disabled
