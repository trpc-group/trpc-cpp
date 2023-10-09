//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/latch_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_latch.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/chrono/chrono.h"

namespace trpc {

std::atomic<bool> exiting{false};

void RunTest() {
  std::atomic<std::size_t> local_count = 0, remote_count = 0;
  while (!exiting) {
    FiberLatch l(1);
    auto called = std::make_shared<std::atomic<bool>>(false);
    Fiber([called, &l, &remote_count] {
      if (!called->exchange(true)) {
        FiberYield();
        l.CountDown();
        ++remote_count;
      }
    }).Detach();
    FiberYield();
    if (!called->exchange(true)) {
      l.CountDown();
      ++local_count;
    }
    l.Wait();
  }
  std::cout << local_count << " " << remote_count << std::endl;
}

TEST(Latch, Torture) {
  RunAsFiber([] {
    Fiber fs[10];
    for (auto&& f : fs) {
      f = Fiber(RunTest);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exiting = true;
    for (auto&& f : fs) {
      f.Join();
    }
  });
}

TEST(Latch, CountDownTwo) {
  RunAsFiber([] {
    FiberLatch l(2);
    ASSERT_FALSE(l.TryWait());
    l.ArriveAndWait(2);
    ASSERT_TRUE(1);
    ASSERT_TRUE(l.TryWait());
  });
}

TEST(Latch, WaitFor) {
  RunAsFiber([] {
    FiberLatch l(1);
    ASSERT_FALSE(l.WaitFor(std::chrono::milliseconds(100)));
    l.CountDown();
    ASSERT_TRUE(l.WaitFor(std::chrono::milliseconds(0)));
  });
}

TEST(Latch, WaitUntil) {
  RunAsFiber([] {
    FiberLatch l(1);
    ASSERT_FALSE(l.WaitUntil(ReadSteadyClock() + std::chrono::milliseconds(100)));
    l.CountDown();
    ASSERT_TRUE(l.WaitUntil(ReadSteadyClock()));
  });
}

}  // namespace trpc
