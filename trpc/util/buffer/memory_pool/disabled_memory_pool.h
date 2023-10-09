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

namespace detail {

/// @brief Memory blocks that store data in the memory pool.
struct alignas(64) Block {
  std::atomic<std::uint32_t> ref_count{1};  ///< Reference counting, used for smart pointer implementations.
  char* data{nullptr};  ///< The memory address of the data, the actual address used to store business data.
};

}  // namespace detail

/// @brief Allocate a Block object for storing data
/// @return Block pointer
detail::Block* Allocate() noexcept;

/// @brief Freeing a memory block.
/// @param block Block pointer
void Deallocate(detail::Block* block) noexcept;

}  // namespace trpc::memory_pool::disabled
