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

#include "trpc/util/buffer/memory_pool/memory_pool.h"

#include <unistd.h>

#include <iostream>
#include <thread>

#include "gtest/gtest.h"

namespace trpc::memory_pool {

namespace testing {

TEST(MemoryPool, AllocateAndDeallocateTest) {
  MemBlock* block = Allocate();
  ASSERT_TRUE(block != nullptr);
  const MemStatistics& stat = GetMemStatistics();
  Deallocate(block);
  PrintMemStatistics();
}

TEST(MemoryPool, GetBlockMaxAvailableSizeTest) { ASSERT_TRUE(GetBlockMaxAvailableSize() > 0); }

TEST(MemoryPool, MakeBlockRefTest) {
  MemBlock* block = Allocate();
  RefPtr<MemBlock> ref_block = MakeBlockRef(block);
  ASSERT_TRUE(ref_block.get() == block);
}

}  // namespace testing
}  // namespace trpc::memory_pool
