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

#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/check.h"

namespace trpc::testing {

#ifndef NDEBUG
#define DASSERT_DEATH(...) ASSERT_DEATH(__VA_ARGS__)
#else
#define DASSERT_DEATH(...)
#endif

TEST(Check, trpc_check) {
  TRPC_CHECK(true, "");
  ASSERT_DEATH(TRPC_CHECK(false), "");

  TRPC_CHECK_EQ(1, 1, "");
  ASSERT_DEATH(TRPC_CHECK_EQ(1, 2), "");

  TRPC_CHECK_NE(1, -1, "");
  ASSERT_DEATH(TRPC_CHECK_NE(1, 1), "");

  TRPC_CHECK_LE(1, 2, "");
  ASSERT_DEATH(TRPC_CHECK_LE(2, 1), "");

  TRPC_CHECK_LT(1, 2, "");
  ASSERT_DEATH(TRPC_CHECK_LT(2, 1), "");

  TRPC_CHECK_GE(2, 1, "");
  ASSERT_DEATH(TRPC_CHECK_GE(1, 2), "");

  TRPC_CHECK_GT(2, 1, "");
  ASSERT_DEATH(TRPC_CHECK_GT(1, 2), "");

  TRPC_CHECK_NEAR(1, 2, 1, "");
  ASSERT_DEATH(TRPC_CHECK_NEAR(1, 2, 0), "");
}

TEST(Check, d_check) {
  TRPC_DCHECK(true, "");
  DASSERT_DEATH(TRPC_DCHECK(false), "");

  TRPC_DCHECK_EQ(1, 1, "");
  DASSERT_DEATH(TRPC_DCHECK_EQ(1, 2), "");

  TRPC_DCHECK_NE(1, -1, "");
  DASSERT_DEATH(TRPC_DCHECK_NE(1, 1), "");

  TRPC_DCHECK_LE(1, 2, "");
  DASSERT_DEATH(TRPC_DCHECK_LE(2, 1), "");

  TRPC_DCHECK_LT(1, 2, "");
  DASSERT_DEATH(TRPC_DCHECK_LT(2, 1), "");

  TRPC_DCHECK_GE(2, 1, "");
  DASSERT_DEATH(TRPC_DCHECK_GE(1, 2), "");

  TRPC_DCHECK_GT(2, 1, "");
  DASSERT_DEATH(TRPC_DCHECK_GT(1, 2), "");

  TRPC_DCHECK_NEAR(1, 2, 1, "");
  DASSERT_DEATH(TRPC_DCHECK_NEAR(1, 2, 0), "");
}

TEST(Check, p_check) {
  TRPC_PCHECK(true, "");
  ASSERT_DEATH(TRPC_PCHECK(false), "");
}

TEST(Check, formatLog) {
  auto out = trpc::detail::log::FormatLog("{}", 123);
  EXPECT_EQ(out, "123");
  auto empty = trpc::detail::log::FormatLog();
  EXPECT_EQ(empty, "");
}

}  // namespace trpc::testing
