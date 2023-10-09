//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/fiber_local_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_local.h"

#include <atomic>
#include <chrono>
#include <vector>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/random.h"

namespace trpc {

TEST(FiberLocal, All) {
  for (int k = 0; k != 10; ++k) {
    RunAsFiber([] {
      static FiberLocal<int> fls;
      static FiberLocal<int> fls2;
      static FiberLocal<double> fls3;
      static FiberLocal<std::vector<int>> fls4;
      constexpr auto N = 10000;

      std::atomic<std::size_t> run{};
      std::vector<Fiber> fs(N);

      for (int i = 0; i != N; ++i) {
        fs[i] = Fiber([i, &run] {
          *fls = i;
          *fls2 = i * 2;
          *fls3 = i + 3;
          fls4->push_back(123);
          fls4->push_back(456);
          while (Random(20) > 1) {
            FiberSleepFor(Random(1000) * std::chrono::microseconds(1));
            ASSERT_EQ(i, *fls);
            ASSERT_EQ(i * 2, *fls2);
            ASSERT_EQ(i + 3, *fls3);
            ASSERT_THAT(*fls4, ::testing::ElementsAre(123, 456));
          }
          ++run;
        });
      }

      for (auto&& e : fs) {
        ASSERT_TRUE(e.Joinable());
        e.Join();
      }

      ASSERT_EQ(N, run);
    });
  }
}

}  // namespace trpc
