//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/erased_ptr_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/erased_ptr.h"

#include "gtest/gtest.h"

namespace trpc::testing {

struct C {
  C() { ++instances; }
  ~C() { --instances; }
  inline static int instances = 0;
};

TEST(ErasedPtr, All) {
  ASSERT_EQ(0, C::instances);
  {
    ErasedPtr ptr(new C{});
    ASSERT_EQ(1, C::instances);
    auto deleter = ptr.GetDeleter();
    auto p = ptr.Leak();
    ASSERT_EQ(1, C::instances);
    deleter(p);
    ASSERT_EQ(0, C::instances);
  }
  ASSERT_EQ(0, C::instances);
  {
    ErasedPtr ptr(new C{});
    ASSERT_EQ(1, C::instances);
  }
  ASSERT_EQ(0, C::instances);
}

}  // namespace trpc::testing
