//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/coroutine/fiber_timed_mutex.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc {

TEST(FiberTimedMutex, TryLockForWhileUnlocked) {
  RunAsFiber([] {
    FiberTimedMutex mutex;
    EXPECT_TRUE(mutex.try_lock_for(std::chrono::milliseconds(0)));
  });
}

TEST(FiberTimedMutex, TryLockForWhileLockedThenFailureOnTimeout) {
  RunAsFiber([] {
    FiberTimedMutex mutex;
    mutex.lock();
    EXPECT_FALSE(mutex.try_lock_for(std::chrono::milliseconds(1)));
  });
}

TEST(FiberTimedMutex, TryLockForWhileLockedThenSuccessBeforeTimeout) {
  RunAsFiber([] {
    FiberTimedMutex mutex;
    mutex.lock();
    Fiber fiber([&mutex] {
      trpc::FiberSleepFor(std::chrono::milliseconds(1));
      mutex.unlock();
    });
    // There is a small probability that try_lock_for will be executed after unlock. In this case, it is equivalent to
    // degrading the test to try_lock_for under a lock-free situation.
    // We ignore this case here because it not worth the effort to solve it.
    EXPECT_TRUE(mutex.try_lock_for(std::chrono::milliseconds(10)));
    fiber.Join();
  });
}

TEST(FiberTimedMutex, TryLockUntilWhileUnlocked) {
  RunAsFiber([] {
    FiberTimedMutex mutex;
    EXPECT_TRUE(mutex.try_lock_until(std::chrono::steady_clock::now()));
  });
}

TEST(FiberTimedMutex, TryLockUntilWhileLockedThenFailureOnTimeout) {
  RunAsFiber([] {
    FiberTimedMutex mutex;
    mutex.lock();
    EXPECT_FALSE(mutex.try_lock_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(1)));
  });
}

TEST(FiberTimedMutex, TryLockUntilWhileLockedThenSuccessBeforeTimeout) {
  RunAsFiber([] {
    FiberTimedMutex mutex;
    mutex.lock();
    Fiber fiber([&mutex] {
      trpc::FiberSleepFor(std::chrono::milliseconds(1));
      mutex.unlock();
    });
    // There is a small probability that try_lock_until will be executed after unlock. In this case, it is equivalent to
    // degrading the test to try_lock_until under a lock-free situation.
    // We ignore this case here because it not worth the effort to solve it.
    EXPECT_TRUE(mutex.try_lock_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(100)));
    fiber.Join();
  });
}

}  // namespace trpc
