//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/index_alloc_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/index_alloc.h"

#include "gtest/gtest.h"

namespace trpc::internal::testing {

struct Tag1;
struct Tag2;

TEST(IndexAlloc, All) {
  auto&& ia1 = IndexAlloc::For<Tag1>();
  auto&& ia2 = IndexAlloc::For<Tag2>();
  ASSERT_EQ(0, ia1->Next());
  ASSERT_EQ(1, ia1->Next());
  ASSERT_EQ(0, ia2->Next());
  ASSERT_EQ(2, ia1->Next());
  ia1->Free(1);
  ASSERT_EQ(1, ia2->Next());
  ASSERT_EQ(1, ia1->Next());
  ASSERT_EQ(2, ia2->Next());
}

}  // namespace trpc::internal::testing
