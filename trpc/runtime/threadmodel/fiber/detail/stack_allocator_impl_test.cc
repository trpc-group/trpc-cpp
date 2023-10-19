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

#include "trpc/runtime/threadmodel/fiber/detail/stack_allocator_impl.h"

#include <unistd.h>

#include <iostream>
#include <thread>

#include "gtest/gtest.h"

namespace trpc::fiber::detail {

TEST(StackAllocatorImpl, InSameThread) {
  uint32_t fiber_stack_size = 131072;
  SetFiberStackSize(fiber_stack_size);

  void* fiber_stack_ptr = nullptr;
  bool is_system = true;

  bool succ = Allocate(&fiber_stack_ptr, &is_system);

  ASSERT_TRUE(succ);
  ASSERT_TRUE(fiber_stack_ptr != nullptr);
  ASSERT_TRUE(!is_system);

  memset(fiber_stack_ptr, 0, fiber_stack_size);

  Deallocate(fiber_stack_ptr, is_system);

  Statistics& stat = GetTlsStatistics();
  ASSERT_TRUE(stat.total_allocs_num == 1);
  ASSERT_TRUE(stat.total_frees_num == 1);
}

TEST(StackAllocatorImpl, InDifferentThread) {
  uint32_t fiber_stack_size = 131072;
  SetFiberStackSize(fiber_stack_size);

  Statistics& stat = GetTlsStatistics();
  size_t allocs_num = stat.total_allocs_num;
  size_t frees_num = stat.total_frees_num;

  void* fiber_stack_ptr = nullptr;
  bool is_system = true;

  bool succ = Allocate(&fiber_stack_ptr, &is_system);

  ASSERT_TRUE(succ);
  ASSERT_TRUE(fiber_stack_ptr != nullptr);
  ASSERT_TRUE(!is_system);

  memset(fiber_stack_ptr, 0, fiber_stack_size);

  std::thread t([fiber_stack_ptr, is_system] () {
    Statistics& stat = GetTlsStatistics();

    Deallocate(fiber_stack_ptr, is_system);

    ASSERT_TRUE(stat.total_allocs_num == 0);
    ASSERT_TRUE(stat.total_frees_num == 1);
  });

  t.join();

  ASSERT_TRUE((stat.total_allocs_num - allocs_num) == 1);
  ASSERT_TRUE((stat.total_frees_num - frees_num) == 0);
}

struct FiberAllocResut {
  void* fiber_stack_ptr = nullptr;
  bool is_system = true;
};

TEST(StackAllocatorImpl, InSameThreadBySystem) {
  Statistics& stat = GetTlsStatistics();

  size_t allocs_num = stat.total_allocs_num;
  size_t frees_num = stat.total_frees_num;

  uint32_t fiber_stack_size = 131072;
  SetFiberStackSize(fiber_stack_size);
  SetFiberStackEnableGuardPage(true);
  SetFiberPoolNumByMmap(30 * 1024);
  SetEnableGdbDebug(true);

  uint32_t alloc_fiber_num = 65536;
  std::vector<FiberAllocResut> alloc_results;
  for (size_t i = 0; i < alloc_fiber_num; ++i) {
    void* fiber_stack_ptr = nullptr;
    bool is_system = true;

    bool succ = Allocate(&fiber_stack_ptr, &is_system);

    ASSERT_TRUE(succ);
    ASSERT_TRUE(fiber_stack_ptr != nullptr);

    memset(fiber_stack_ptr, 0, fiber_stack_size);

    FiberAllocResut alloc_result;
    alloc_result.fiber_stack_ptr = fiber_stack_ptr;
    alloc_result.is_system = is_system;

    alloc_results.push_back(alloc_result);
  }

  for (size_t i = 0; i < alloc_fiber_num; ++i) {
    FiberAllocResut& alloc_result = alloc_results[i];

    Deallocate(alloc_result.fiber_stack_ptr, alloc_result.is_system);
  }

  PrintTlsStatistics();

  ASSERT_TRUE((stat.total_allocs_num - allocs_num) == alloc_fiber_num);
  ASSERT_TRUE((stat.total_frees_num - frees_num) == alloc_fiber_num);
}

TEST(StackAllocatorImpl, PrewarmFiberPool) {
  int prewarm_nun = PrewarmFiberPool(1024);

  ASSERT_TRUE(prewarm_nun == 1024);
}

}  // namespace trpc::fiber::detail
