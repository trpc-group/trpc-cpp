//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/cpu_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/cpu.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(GetCurrentNode, All) { EXPECT_GE(trpc::numa::GetCurrentNode(), 0); }

TEST(GetNodeIndexOf, All) {
  auto index = trpc::numa::GetNodeIndexOf(std::numeric_limits<int>::max());
  ASSERT_EQ(index, trpc::numa::kUnexpectedNodeIndex);
}

TEST(TryParseProcesserList, All) {
  {
    auto ret = TryParseProcesserList("-200000");
    ASSERT_FALSE(ret);
  }

  {
    auto ret = TryParseProcesserList("-1");
    ASSERT_TRUE(ret);

    int rc = GetNumberOfProcessorsConfigured();
    ASSERT_EQ(ret.value(), std::vector<unsigned>({static_cast<unsigned>(rc - 1)}));
  }

  {
    auto ret = TryParseProcesserList("2");
    ASSERT_TRUE(ret);
    ASSERT_EQ(ret.value(), std::vector<unsigned>({2}));
  }

  {
    auto ret = TryParseProcesserList("1-");
    ASSERT_FALSE(ret);
  }

  {
    auto ret = TryParseProcesserList("2-1");
    ASSERT_FALSE(ret);
  }

  {
    auto ret = TryParseProcesserList("1-2, 5, 9-11");
    ASSERT_TRUE(ret);
    ASSERT_EQ(ret.value(), std::vector<unsigned>({1, 2, 5, 9, 10, 11}));
  }
}

}  // namespace trpc::testing
