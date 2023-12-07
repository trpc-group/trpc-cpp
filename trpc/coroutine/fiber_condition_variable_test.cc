//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/condition_variable_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_condition_variable.h"

#include <atomic>
#include <chrono>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/random.h"

namespace trpc {

TEST(FiberConditionVariable, UseInFiberContext) {
  RunAsFiber([] {
    for (int k = 0; k != 10; ++k) {
      constexpr auto N = 600;

      std::atomic<std::size_t> run{};
      FiberMutex lock[N];
      FiberConditionVariable cv[N];
      bool set[N];
      std::vector<Fiber> prod(N);
      std::vector<Fiber> cons(N);

      for (int i = 0; i != N; ++i) {
        prod[i] = Fiber([&run, i, &cv, &lock, &set] {
          FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
          std::unique_lock lk(lock[i]);
          cv[i].wait(lk, [&] { return set[i]; });
          ++run;
        });
        cons[i] = Fiber([&run, i, &cv, &lock, &set] {
          FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
          std::scoped_lock _(lock[i]);
          set[i] = true;
          cv[i].notify_one();
          ++run;
        });
      }

      for (auto&& e : prod) {
        ASSERT_TRUE(e.Joinable());
        e.Join();
      }
      for (auto&& e : cons) {
        ASSERT_TRUE(e.Joinable());
        e.Join();
      }

      ASSERT_EQ(N * 2, run);
    }
  });
}

TEST(FiberConditionVariable, UseInPthreadContext) {
  constexpr auto N = 64;
  std::atomic<std::size_t> run{0};
  FiberMutex lock[N];
  FiberConditionVariable cv[N];
  bool set[N] = {false};
  std::vector<std::thread> prod(N);
  std::vector<std::thread> cons(N);

  for (int i = 0; i != N; ++i) {
    prod[i] = std::thread([&run, i, &cv, &lock, &set] {
      FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
      std::unique_lock lk(lock[i]);
      cv[i].wait(lk, [&] { return set[i]; });
      ++run;
    });

    cons[i] = std::thread([&run, i, &cv, &lock, &set] {
      FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
      std::scoped_lock _(lock[i]);
      set[i] = true;
      cv[i].notify_one();
      ++run;
    });
  }

  for (auto&& e : prod) {
    ASSERT_TRUE(e.joinable());
    e.join();
  }

  for (auto&& e : cons) {
    ASSERT_TRUE(e.joinable());
    e.join();
  }

  ASSERT_EQ(N * 2, run);
}

TEST(FiberConditionVariable, NotifyPthreadFromFiber) {
  RunAsFiber([] {
    constexpr auto N = 64;
    std::atomic<std::size_t> run{0};
    FiberMutex lock[N];
    FiberConditionVariable cv[N];
    bool set[N] = {false};
    std::vector<std::thread> prod(N);
    std::vector<Fiber> cons(N);

    for (int i = 0; i != N; ++i) {
      prod[i] = std::thread([&run, i, &cv, &lock, &set] {
        FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
        std::unique_lock lk(lock[i]);
        cv[i].wait(lk, [&] { return set[i]; });
        ++run;
      });

      cons[i] = Fiber([&run, i, &cv, &lock, &set] {
        FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
        std::scoped_lock _(lock[i]);
        set[i] = true;
        cv[i].notify_one();
        ++run;
      });
    }

    for (auto&& e : prod) {
      ASSERT_TRUE(e.joinable());
      e.join();
    }

    for (auto&& e : cons) {
      ASSERT_TRUE(e.Joinable());
      e.Join();
    }

    ASSERT_EQ(N * 2, run);
  });
}

TEST(FiberConditionVariable, NotifyFiberFromPthread) {
  RunAsFiber([] {
    constexpr auto N = 64;
    std::atomic<std::size_t> run{0};
    FiberMutex lock[N];
    FiberConditionVariable cv[N];
    bool set[N] = {false};
    std::vector<Fiber> prod(N);
    std::vector<std::thread> cons(N);

    for (int i = 0; i != N; ++i) {
      prod[i] = Fiber([&run, i, &cv, &lock, &set] {
        FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
        std::unique_lock lk(lock[i]);
        cv[i].wait(lk, [&] { return set[i]; });
        ++run;
      });

      cons[i] = std::thread([&run, i, &cv, &lock, &set] {
        FiberSleepFor(Random(20) * std::chrono::milliseconds(1));
        std::scoped_lock _(lock[i]);
        set[i] = true;
        cv[i].notify_one();
        ++run;
      });
    }

    for (auto&& e : prod) {
      ASSERT_TRUE(e.Joinable());
      e.Join();
    }

    for (auto&& e : cons) {
      ASSERT_TRUE(e.joinable());
      e.join();
    }

    ASSERT_EQ(N * 2, run);
  });
}

}  // namespace trpc
