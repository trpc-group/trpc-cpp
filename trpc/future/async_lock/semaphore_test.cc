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

#include "trpc/future/async_lock/semaphore.h"

#include "gtest/gtest.h"

namespace trpc {

/// @brief Test init.
TEST(Semaphore, Init) {
  Semaphore sem(Semaphore::MaxCounter());
  EXPECT_GT(sem.AvailableUnits(), 0);

  // Not in any thread model.
  EXPECT_EQ(sem.WorkerId(), -1);

  Semaphore sem1(10);
  EXPECT_EQ(sem1.AvailableUnits(), 10);
}

/// @brief Test for free sem.
TEST(Semaphore, WaitWithFreeSemaphore) {
  Semaphore sem(10);
  auto f = sem.Wait(1);

  // Free sem and no waiter, f should be ready.
  EXPECT_TRUE(f.IsReady());
  EXPECT_FALSE(f.IsFailed());

  f.Then([&sem]() {
    EXPECT_EQ(sem.AvailableUnits(), 9);
    return MakeReadyFuture();
  })
  .Then([&sem]() {
    sem.Signal(1);
    EXPECT_EQ(sem.AvailableUnits(), 10);
  });
}

/// @brief Test wait for sem with no available unit.
TEST(Semaphore, WaitSemaphoreWithNoAvailableUnit) {
  Semaphore sem(3);
  auto f1 = sem.Wait(2);
  EXPECT_EQ(sem.AvailableUnits(), 1);
  auto f2 = sem.Wait(1);
  EXPECT_EQ(sem.AvailableUnits(), 0);

  // f1 & f2 should both be ready, due to unit-availability
  EXPECT_TRUE(f1.IsReady());
  EXPECT_FALSE(f1.IsFailed());

  EXPECT_TRUE(f2.IsReady());
  EXPECT_FALSE(f2.IsFailed());

  auto f3 = sem.Wait(3);
  EXPECT_FALSE(f3.IsReady());
  EXPECT_FALSE(f3.IsFailed());

  auto f4 = sem.Wait(1);
  EXPECT_FALSE(f3.IsReady());
  EXPECT_FALSE(f3.IsFailed());

  // 2 in wait list
  EXPECT_EQ(sem.Waiters(), 2);

  f1.Then([&sem]() {
    sem.Signal(2);
    return MakeReadyFuture();
  });

  // Still not enough.
  EXPECT_EQ(sem.AvailableUnits(), 2);
  EXPECT_FALSE(f3.IsReady());
  EXPECT_FALSE(f3.IsFailed());

  f2.Then([&sem]() {
    sem.Signal(1);
    return MakeReadyFuture();
  });

  // There's enough units for f3, great.
  EXPECT_TRUE(f3.IsReady());
  EXPECT_FALSE(f3.IsFailed());
  // Semaphore became busy again.
  EXPECT_EQ(sem.AvailableUnits(), 0);
  // And f4 is still waiting
  EXPECT_FALSE(f4.IsReady());
  EXPECT_FALSE(f4.IsFailed());
}

/// @brief Test for broke sem.
TEST(Semaphore, BrokeSemaphore) {
  Semaphore sem(3);

  auto f1 = sem.Wait(3);
  EXPECT_EQ(sem.AvailableUnits(), 0);

  auto f2 = sem.Wait(1);
  auto f3 = sem.Wait(3);

  // f1 & f2 are in waiting list
  EXPECT_FALSE(f2.IsReady());
  EXPECT_FALSE(f2.IsFailed());

  EXPECT_FALSE(f3.IsReady());
  EXPECT_FALSE(f3.IsFailed());

  sem.Broke();

  // f2 & f3 should be notified
  // Now they are not ready but failed
  EXPECT_FALSE(f2.IsReady());
  EXPECT_TRUE(f2.IsFailed());
  EXPECT_STREQ(f2.GetException().what(), "Broke the semaphore");

  EXPECT_FALSE(f3.IsReady());
  EXPECT_TRUE(f3.IsFailed());
  EXPECT_STREQ(f3.GetException().what(), "Broke the semaphore");
}

/// @brief Test for broke sem with exception.
TEST(Semaphore, BrokeSemaphoreWithException) {
  Semaphore sem(3);

  auto f1 = sem.Wait(3);
  EXPECT_EQ(sem.AvailableUnits(), 0);

  auto f2 = sem.Wait(1);
  auto f3 = sem.Wait(3);

  // f1 & f2 are in waiting list
  EXPECT_FALSE(f2.IsReady());
  EXPECT_FALSE(f2.IsFailed());

  EXPECT_FALSE(f3.IsReady());
  EXPECT_FALSE(f3.IsFailed());

  sem.Broke("Cancel");

  // f2 & f3 should be notified
  // Now they are not ready but failed
  EXPECT_FALSE(f2.IsReady());
  EXPECT_TRUE(f2.IsFailed());
  EXPECT_STREQ(f2.GetException().what(), "Cancel");

  EXPECT_FALSE(f3.IsReady());
  EXPECT_TRUE(f3.IsFailed());
  EXPECT_STREQ(f3.GetException().what(), "Cancel");
}

/// @brief Test for TryWait.
TEST(Semaphore, TryWait) {
  Semaphore sem(3);

  // Try wait a free semaphore, succeed
  EXPECT_TRUE(sem.TryWait(3));

  // Reset
  sem.Signal(3);

  // Occupy all units
  auto f1 = sem.Wait(3);
  EXPECT_EQ(sem.AvailableUnits(), 0);
  EXPECT_TRUE(f1.IsReady());
  EXPECT_FALSE(f1.IsFailed());

  // No available units
  EXPECT_FALSE(sem.TryWait(1));

  sem.Signal(3);
  EXPECT_EQ(sem.AvailableUnits(), 3);

  // Intentionally block the semaphore
  auto f2 = sem.Wait(4);
  EXPECT_EQ(sem.AvailableUnits(), 3);
  EXPECT_EQ(sem.Waiters(), 1);

  // Enough available units, but there's other waiters
  EXPECT_FALSE(sem.TryWait(2));
}

/// @brief Test for Recursion.
TEST(Semaphore, Recursion) {
  Semaphore sem(Semaphore::MaxCounter());

  EXPECT_TRUE(sem.TryWait(Semaphore::MaxCounter()));

  sem.Wait(Semaphore::MaxCounter()).Then([&](){
    EXPECT_TRUE(sem.Signal(Semaphore::MaxCounter()));
  });
  sem.Wait(Semaphore::MaxCounter()).Then([&](){
    EXPECT_TRUE(sem.Signal(Semaphore::MaxCounter()));
  });
  sem.Wait(Semaphore::MaxCounter()).Then([&](){
    EXPECT_TRUE(sem.Signal(Semaphore::MaxCounter()));
  });

  EXPECT_TRUE(sem.Signal(Semaphore::MaxCounter()));
  EXPECT_EQ(sem.AvailableUnits(), Semaphore::MaxCounter());
}

}  // namespace trpc
