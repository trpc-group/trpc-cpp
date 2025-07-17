//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/waitable_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_mutex.h"

#include <iostream>
#include <mutex>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc {

TEST(FiberMutex, All) {
  RunAsFiber([] {
    FiberMutex mtx;
    int a = 0, b = 0;
    std::vector<Fiber> fibers;
    int i = 8;
    while (i--) {
      fibers.emplace_back([&] {
        int j = 40000;
        while (j--) {
          std::lock_guard<decltype(mtx)> _(mtx);
          a++;
          b++;
          EXPECT_EQ(a, b);
        }
      });
    }
    for (auto&& e : fibers) {
      e.Join();
    }
    EXPECT_EQ(a, 320000);
    EXPECT_EQ(a, b);
  });
}

}  // namespace trpc
