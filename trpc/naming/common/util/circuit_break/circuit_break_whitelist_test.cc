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

#include "trpc/naming/common/util/circuit_break/circuit_break_whitelist.h"

#include "gtest/gtest.h"

namespace trpc::naming::testing {

TEST(CircuitBreakWhiteListTest, SetAndContains) {
  CircuitBreakWhiteList white_list;
  std::vector<int> retcodes;
  retcodes.emplace_back(101);
  retcodes.emplace_back(121);
  white_list.SetCircuitBreakWhiteList(retcodes);
  ASSERT_TRUE(white_list.Contains(101));
  ASSERT_TRUE(white_list.Contains(121));
}

}  // namespace trpc::naming::testing