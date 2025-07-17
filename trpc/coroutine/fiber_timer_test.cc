//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/timer_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_timer.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc {

TEST(Timer, SetTimer) {
  RunAsFiber([] {
    auto start = ReadSteadyClock();
    std::atomic<bool> done{};
    auto timer_id = SetFiberTimer(start + std::chrono::milliseconds(100), [&](auto) {
      ASSERT_NEAR((ReadSteadyClock() - start) / std::chrono::milliseconds(1),
                  std::chrono::milliseconds(100) / std::chrono::milliseconds(1), 10);
      done = true;
    });

    std::atomic<std::size_t> max_sleep{};
    while (!done && max_sleep < 1000) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      max_sleep++;
    }
    KillFiberTimer(timer_id);
  });
}

TEST(Timer, SetPeriodicTimer) {
  RunAsFiber([] {
    auto start = ReadSteadyClock();
    std::atomic<std::size_t> called{};
    auto timer_id = SetFiberTimer(
        start + std::chrono::milliseconds(100), std::chrono::milliseconds(10), [&](auto) {
          ASSERT_NEAR(
              (ReadSteadyClock() - start) / std::chrono::milliseconds(1),
              (std::chrono::milliseconds(100) + called.load() * std::chrono::milliseconds(10)) /
                  std::chrono::milliseconds(1),
              10);
          ++called;
        });

    std::atomic<std::size_t> max_sleep{};
    while (called != 10 && max_sleep < 5000) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      max_sleep++;
    }
    KillFiberTimer(timer_id);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  });
}

TEST(Timer, FiberTimerKiller) {
  RunAsFiber([] {
    auto start = ReadSteadyClock();
    std::atomic<bool> done{};
    FiberTimerKiller killer(SetFiberTimer(start + std::chrono::milliseconds(100), [&](auto) {
      ASSERT_NEAR((ReadSteadyClock() - start) / std::chrono::milliseconds(1),
                  std::chrono::milliseconds(100) / std::chrono::milliseconds(1), 10);
      done = true;
    }));

    std::atomic<std::size_t> max_sleep{};
    while (!done && max_sleep < 1000) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      max_sleep++;
    }
  });
}

TEST(Timer, SetDetachedTimer) {
  RunAsFiber([] {
    auto start = ReadSteadyClock();
    std::atomic<bool> called{};
    SetFiberDetachedTimer(start + std::chrono::milliseconds(100), [&]() {
      ASSERT_NEAR((ReadSteadyClock() - start) / std::chrono::milliseconds(1),
                  std::chrono::milliseconds(100) / std::chrono::milliseconds(1), 10);
      called = true;
    });

    std::atomic<std::size_t> max_sleep{};
    while (!called && max_sleep < 1000) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      max_sleep++;
    }
  });
}

}  // namespace trpc
