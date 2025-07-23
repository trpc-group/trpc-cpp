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

#include "trpc/util/algorithm/power_of_two.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(RoundUpPowerOf2, All) {
  std::size_t n = RoundUpPowerOf2(3);
  ASSERT_EQ(n, 4);
  n = RoundUpPowerOf2(8);
  ASSERT_EQ(n, 8);
}

}  // namespace trpc::testing
