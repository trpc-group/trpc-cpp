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
#include "trpc/util/algorithm/random.h"
#include "trpc/util/chrono/chrono.h"

namespace trpc {

TEST(FiberLatch, WaitInFiberCountDownFromFiber) {
  RunAsFiber([]() {
    for (int i = 0; i != 100; ++i) {
      auto fl = std::make_unique<trpc::FiberLatch>(1);
      trpc::Fiber fb = trpc::Fiber([&] {
        trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
        fl->Wait();
      });
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      fl->CountDown();
      fb.Join();
    }
  });
}

TEST(FiberLatch, WaitInFiberCountDownFromPthread) {
  RunAsFiber([]() {
    for (int i = 0; i != 100; ++i) {
      auto fl = std::make_unique<trpc::FiberLatch>(1);
      std::thread t([&] {
        trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
        fl->CountDown();
      });
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      fl->Wait();
      t.join();
    }
  });
}

TEST(FiberLatch, WaitInPthreadCountDownFromFiber) {
  RunAsFiber([]() {
    for (int i = 0; i != 100; ++i) {
      auto fl = std::make_unique<trpc::FiberLatch>(1);
      std::thread t([&] {
        trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
        fl->Wait();
      });
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      fl->CountDown();
      t.join();
    }
  });
}

TEST(FiberLatch, WaitInPthreadCountDownFromPthread) {
  for (int i = 0; i != 100; ++i) {
    auto fl = std::make_unique<trpc::FiberLatch>(1);
    std::thread t([&] {
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      fl->Wait();
    });
    trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
    fl->CountDown();
    t.join();
  }
}

TEST(FiberLatch, CountDownTwoInFiber) {
  RunAsFiber([] {
    FiberLatch l(2);
    ASSERT_FALSE(l.TryWait());
    l.ArriveAndWait(2);
    ASSERT_TRUE(l.TryWait());
  });
}

TEST(FiberLatch, CountDownTwoInPthread) {
  FiberLatch l(2);
  ASSERT_FALSE(l.TryWait());
  l.ArriveAndWait(2);
  ASSERT_TRUE(l.TryWait());
}

TEST(FiberLatch, WaitForInFiber) {
  RunAsFiber([] {
    FiberLatch l(1);
    ASSERT_FALSE(l.WaitFor(std::chrono::milliseconds(100)));
    l.CountDown();
    ASSERT_TRUE(l.WaitFor(std::chrono::milliseconds(0)));
  });
}

TEST(FiberLatch, WaitForInPthread) {
  FiberLatch l(1);
  ASSERT_FALSE(l.WaitFor(std::chrono::milliseconds(100)));
  l.CountDown();
  ASSERT_TRUE(l.WaitFor(std::chrono::milliseconds(0)));
}

TEST(FiberLatch, WaitUntilInFiber) {
  RunAsFiber([] {
    FiberLatch l(1);
    ASSERT_FALSE(l.WaitUntil(ReadSteadyClock() + std::chrono::milliseconds(100)));
    l.CountDown();
    ASSERT_TRUE(l.WaitUntil(ReadSteadyClock()));
  });
}

TEST(FiberLatch, WaitUntilInPthread) {
  FiberLatch l(1);
  ASSERT_FALSE(l.WaitUntil(ReadSteadyClock() + std::chrono::milliseconds(100)));
  l.CountDown();
  ASSERT_TRUE(l.WaitUntil(ReadSteadyClock()));
}

}  // namespace trpc
