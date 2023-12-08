//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/waitable_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_event.h"

#include <atomic>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/algorithm/random.h"

namespace trpc::testing {

TEST(FiberEvent, WaitInFiberSetFromPthread) {
  RunAsFiber([]() {
    for (int i = 0; i != 1000; ++i) {
      auto ev = std::make_unique<trpc::FiberEvent>();
      std::thread t([&] {
        // Random sleep to make Set before Wait or after Wait.
        trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
        ev->Set();
      });
      // Random sleep to make Wait return immediately or awakened by Set.
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      ev->Wait();
      t.join();
    }
  });
}

TEST(FiberEvent, WaitInFiberSetFromFiber) {
  RunAsFiber([]() {
    for (int i = 0; i != 1000; ++i) {
      auto ev = std::make_unique<trpc::FiberEvent>();
      trpc::Fiber f = trpc::Fiber([&] {
        trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
        ev->Wait();
      });
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      ev->Set();
      f.Join();
    }
  });
}

TEST(FiberEvent, WaitInPthreadSetFromFiber) {
  RunAsFiber([]() {
    for (int i = 0; i != 1000; ++i) {
      auto ev = std::make_unique<trpc::FiberEvent>();
      std::thread t([&] {
        trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
        ev->Wait();
      });
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      ev->Set();
      t.join();
    }
  });
}

TEST(FiberEvent, WaitInPthreadSetFromPthread) {
  for (int i = 0; i != 1000; ++i) {
    auto ev = std::make_unique<trpc::FiberEvent>();
    std::thread t([&] {
      trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
      ev->Wait();
    });
    trpc::FiberSleepFor(Random(10) * std::chrono::milliseconds(1));
    ev->Set();
    t.join();
  }
}

}  // namespace trpc::testing
