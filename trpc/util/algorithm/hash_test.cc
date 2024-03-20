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

#include "trpc/util/algorithm/hash.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(GetHashIndex, All) {
  std::uint64_t key = 10;
  std::uint64_t mod = 8;
  std::size_t ret = GetHashIndex(key, mod);
  ASSERT_LE(ret, mod);
  // Test case: mod is 0.
  mod = 0;
  ASSERT_DEATH(GetHashIndex(key, mod), "");
}

}  // namespace trpc::testing
