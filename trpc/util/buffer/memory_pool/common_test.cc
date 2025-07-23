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

#include <stdlib.h>

#include "gtest/gtest.h"

namespace trpc::memory_pool {

namespace testing {

void* MockAllocateMemFunc(std::size_t alignment, std::size_t size) { return aligned_alloc(alignment, size); }
void MockDeallocateMemFunc(void* ptr) { free(ptr); }

TEST(AllocateMemFunc, SetAndGet) {
  auto alloc_func = GetAllocateMemFunc();
  ASSERT_EQ(alloc_func, aligned_alloc);

  SetAllocateMemFunc(MockAllocateMemFunc);
  alloc_func = GetAllocateMemFunc();
  ASSERT_EQ(alloc_func, MockAllocateMemFunc);
}

TEST(DeallocateMemFunc, SetAndGet) {
  auto dealloc_func = GetDeallocateMemFunc();
  ASSERT_EQ(dealloc_func, free);

  SetDeallocateMemFunc(MockDeallocateMemFunc);
  dealloc_func = GetDeallocateMemFunc();
  ASSERT_EQ(dealloc_func, MockDeallocateMemFunc);
}

TEST(MemBlockSize, SetAndGet) {
  ASSERT_EQ(GetMemBlockSize(), kDefaultBlockSize);
  SetMemBlockSize(0);
  ASSERT_EQ(GetMemBlockSize(), kDefaultBlockSize);

  SetMemBlockSize(1024);
  ASSERT_EQ(GetMemBlockSize(), 1024);
}

TEST(MemPoolThreshold, SetAndGet) {
  ASSERT_EQ(GetMemPoolThreshold(), kDefaultMemPoolThreshold);
  SetMemPoolThreshold(0);
  ASSERT_EQ(GetMemPoolThreshold(), kDefaultMemPoolThreshold);

  SetMemPoolThreshold(128 * 1024 * 1024);
  ASSERT_EQ(GetMemPoolThreshold(), 128 * 1024 * 1024);
}

}  // namespace testing

}  // namespace trpc::memory_pool
