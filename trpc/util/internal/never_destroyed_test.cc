//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/never_destroyed_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/never_destroyed.h"

#include "gtest/gtest.h"

namespace trpc::internal::testing {

struct C {
  C() { ++instances; }
  ~C() { --instances; }
  inline static std::size_t instances{};
};

struct D {
  void foo() { [[maybe_unused]] static NeverDestroyedSingleton<D> test_compilation2; }
};

NeverDestroyed<int> test_compilation2;

TEST(NeverDestroyed, All) {
  ASSERT_EQ(0, C::instances);
  {
    C c1;
    ASSERT_EQ(1, C::instances);
    [[maybe_unused]] NeverDestroyed<C> c2;
    ASSERT_EQ(2, C::instances);
  }
  // Not 0, as `NeverDestroyed<C>` is not destroyed.
  ASSERT_EQ(1, C::instances);
}

}  // namespace trpc::internal::testing
