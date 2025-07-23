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

#include "trpc/util/buffer/memory_pool/global_memory_pool.h"

#include <unistd.h>

#include <iostream>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/util/buffer/memory_pool/common.h"

namespace trpc::memory_pool::global {

namespace testing {

TEST(BlockAllocatorImpl, InSameThread) {
  detail::Block* block = Allocate();

  ASSERT_TRUE(block);

  Deallocate(block);

  const Statistics& stat = GetTlsStatistics();
  ASSERT_TRUE(stat.total_allocs_num == 1);
  ASSERT_TRUE(stat.total_frees_num == 1);
}

TEST(BlockAllocatorImpl, InDifferentThread) {
  const Statistics& stat = GetTlsStatistics();
  size_t allocs_num = stat.total_allocs_num;
  size_t frees_num = stat.total_frees_num;

  detail::Block* block = Allocate();

  ASSERT_TRUE(block);

  std::thread t([block]() {
    const Statistics& stat = GetTlsStatistics();

    Deallocate(block);

    ASSERT_TRUE(stat.total_allocs_num == 0);
    ASSERT_TRUE(stat.total_frees_num == 1);
  });

  t.join();

  ASSERT_TRUE((stat.total_allocs_num - allocs_num) == 1);
  ASSERT_TRUE((stat.total_frees_num - frees_num) == 0);
}

TEST(BlockAllocatorImpl, InSameThreadBySystem) {
  const Statistics& stat = GetTlsStatistics();

  size_t allocs_num = stat.total_allocs_num;
  size_t frees_num = stat.total_frees_num;

  uint32_t alloc_fiber_num = GetMemPoolThreshold() / GetMemBlockSize();

  std::vector<detail::Block*> alloc_results;
  for (size_t i = 0; i < alloc_fiber_num; ++i) {
    detail::Block* block = Allocate();

    ASSERT_TRUE(block != nullptr);

    alloc_results.push_back(block);
  }

  for (size_t i = 0; i < alloc_fiber_num; ++i) {
    detail::Block* block = alloc_results[i];

    Deallocate(block);
  }

  PrintTlsStatistics();

  ASSERT_TRUE((stat.total_allocs_num - allocs_num) == alloc_fiber_num);
  ASSERT_TRUE((stat.total_frees_num - frees_num) == alloc_fiber_num);
}

TEST(BlockAllocatorImpl, PrewarmMemPool) {
  int prewarm_nun = PrewarmMemPool(1024);

  ASSERT_TRUE(prewarm_nun == 1024);
}
}  // namespace testing
}  // namespace trpc::memory_pool::global
