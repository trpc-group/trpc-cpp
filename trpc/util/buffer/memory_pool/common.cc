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

#include "trpc/util/buffer/memory_pool/common.h"

#include <malloc.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>

#include "trpc/util/algorithm/power_of_two.h"

namespace trpc::memory_pool {

namespace {

// The default memory allocation uses system allocation.
static AllocateMemFunc s_block_mem_allocate = ::aligned_alloc;
// The default memory deallocation uses the system's.
static DeallocateMemFunc s_block_mem_deallocate = ::free;

// Each block is typically 4 kilobytes by default.
static std::size_t s_block_size = kDefaultBlockSize;

// The default threshold for the memory pool is 512 megabytes.
static std::size_t s_mem_pool_threshold = kDefaultMemPoolThreshold;

}  // namespace

void SetAllocateMemFunc(AllocateMemFunc allocate) { s_block_mem_allocate = allocate; }
AllocateMemFunc GetAllocateMemFunc() { return s_block_mem_allocate; }

void SetDeallocateMemFunc(DeallocateMemFunc deallocate) { s_block_mem_deallocate = deallocate; }
DeallocateMemFunc GetDeallocateMemFunc() { return s_block_mem_deallocate; }

void SetMemBlockSize(std::size_t size) {
  if (size == 0) {
    // Using default values.
    size = kDefaultBlockSize;
  }
  // Round up to the nearest power of 2.
  s_block_size = RoundUpPowerOf2(size);
}
std::size_t GetMemBlockSize() { return s_block_size; }

void SetMemPoolThreshold(std::size_t size) {
  if (size == 0) {
    // Using default values.
    size = kDefaultMemPoolThreshold;
  }
  // Round up to the nearest power of 2.
  s_mem_pool_threshold = RoundUpPowerOf2(size);
}
std::size_t GetMemPoolThreshold() { return s_mem_pool_threshold; }

}  // namespace trpc::memory_pool
