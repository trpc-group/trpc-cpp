//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/shared_mutex_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_shared_mutex.h"

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/latch.h"
#include "trpc/util/likely.h"
#include "trpc/util/random.h"
#include "trpc/util/time.h"

namespace trpc {

int counter1, counter2;

TEST(FiberSharedMutex, Simple) {
  RunAsFiber([] {
    FiberSharedMutex rwlock;
    for (int i = 0; i != 100000; ++i) {
      std::shared_lock _(rwlock);
    }
    for (int i = 0; i != 100000; ++i) {
      std::scoped_lock _(rwlock);
    }
    for (int i = 0; i != 100000; ++i) {
      std::shared_lock _(rwlock);
    }
  });
}

TEST(FiberSharedMutex, All) {
  RunAsFiber([] {
    FiberSharedMutex rwlock;
    std::vector<Fiber> fibers;
    std::atomic<int> try_read_lock{}, try_write_lock{};
    for (int i = 0; i != 10000; ++i) {
      fibers.emplace_back([&] {
        for (int _ = 0; _ != 100; ++_) {
          auto op = Random(100);
          if (op < 90) {
            std::shared_lock _(rwlock);
            EXPECT_EQ(counter1, counter2);
          } else if (op < 95) {
            std::scoped_lock _(rwlock);
            ++counter1;
            ++counter2;
            EXPECT_EQ(counter1, counter2);
          } else if (op < 99) {
            std::shared_lock lk(rwlock, std::try_to_lock);
            if (lk) {
              ++try_read_lock;
              EXPECT_EQ(counter1, counter2);
            }
          } else {
            std::unique_lock lk(rwlock, std::try_to_lock);
            if (lk) {
              ++try_write_lock;
              ++counter1;
              ++counter2;
              EXPECT_EQ(counter1, counter2);
            }
          }
        }
      });
    }
    for (auto&& e : fibers) {
      e.Join();
    }
  });
}

TEST(FiberShardMutex, AllWriter) {
  RunAsFiber([] {
    FiberSharedMutex mtx;
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
