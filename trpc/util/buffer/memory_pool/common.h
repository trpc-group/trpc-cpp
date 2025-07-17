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

#pragma once

#include <cstddef>

namespace trpc::memory_pool {

/// @brief Default memory pool threshold size is 512MB.
static constexpr std::size_t kDefaultMemPoolThreshold = 512 * 1024 * 1024;
/// @brief Default memory block size is 4KB.
static constexpr std::size_t kDefaultBlockSize = 4096;

/// @brief Memory allocation function type.
typedef void* (*AllocateMemFunc)(std::size_t alignment, std::size_t size);
/// @brief Memory deallocation function type.
typedef void (*DeallocateMemFunc)(void*);

/// @brief Registering a memory allocation function that is not thread-safe.
/// @param allocate Custom memory allocation function
void SetAllocateMemFunc(AllocateMemFunc allocate);
/// @brief Getting the registered memory allocation function that is not thread-safe.
/// @return AllocateMemFunc type
AllocateMemFunc GetAllocateMemFunc();

/// @brief Registering a user-defined memory deallocation function that is not thread-safe
/// @param deallocate Custom memory deallocation function.
void SetDeallocateMemFunc(DeallocateMemFunc deallocate);
/// @brief Getting the user-defined memory deallocation function that is not thread-safe.
/// @return DeallocateMemFunc type
DeallocateMemFunc GetDeallocateMemFunc();

/// @brief Setting the size of a memory block that is not thread-safe.
/// @param size Size of a memory block.
void SetMemBlockSize(std::size_t size);
/// @brief Getting the size of a memory block that is not thread-safe.
/// @return std::size_t type
/// @note Because the set size is rounded up to the nearest power of 2, the retrieved size may be larger than the set
///       size.
std::size_t GetMemBlockSize();

/// @brief Setting the size threshold for a memory pool that is not thread-safe.
/// @param size Size threshold for a memory pool.
void SetMemPoolThreshold(std::size_t size);
/// @brief Getting the size threshold for a memory pool that is not thread-safe.
/// @return std::size_t type
/// @note Because the set size is rounded up to the nearest power of 2, the retrieved size may be larger than the set
///       size.
std::size_t GetMemPoolThreshold();

}  // namespace trpc::memory_pool
