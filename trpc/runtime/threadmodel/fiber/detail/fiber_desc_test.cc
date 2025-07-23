//
//
// Copyright (C) 2021 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_desc_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/fiber_desc.h"

#include "gtest/gtest.h"

namespace trpc::fiber::detail {

TEST(FiberDesc, All) {
  auto ptr = NewFiberDesc();
  ASSERT_TRUE(isa<FiberDesc>(ptr));
  DestroyFiberDesc(ptr);
}

}  // namespace trpc::fiber::detail
