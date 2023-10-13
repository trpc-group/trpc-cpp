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

#include "trpc/util/buffer/memory_pool/disabled_memory_pool.h"

#include "gtest/gtest.h"

namespace trpc::memory_pool::disabled {

namespace testing {

TEST(DisabledMemoryPool, Normal) {
  auto* ptr = Allocate();
  ASSERT_TRUE(ptr != nullptr);
  Deallocate(ptr);
  PrintStatistics();
}
}  // namespace testing

}  // namespace trpc::memory_pool::disabled
