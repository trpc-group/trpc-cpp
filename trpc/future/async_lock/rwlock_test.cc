//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/future/async_lock/rwlock.h"

#include "gtest/gtest.h"

namespace trpc {

/// @brief Test for read lock.
TEST(RWLock, RWLockForReadLock) {
  RWLock l;

  auto& r = *(l.ForRead());
  auto f1 = r.Lock();
  auto f2 = r.Lock();
  EXPECT_TRUE(f1.IsReady());
  EXPECT_TRUE(f2.IsReady());
}

/// @brief Test for write lock.
TEST(RWLock, RWLockForWriteLock) {
  RWLock l;

  auto& r = *(l.ForRead());
  auto f1 = r.Lock();
  auto f2 = r.Lock();
  EXPECT_TRUE(f1.IsReady());
  EXPECT_TRUE(f2.IsReady());

  auto& w = *(l.ForWrite());
  auto f3 = w.Lock();
  EXPECT_FALSE(f3.IsReady());

  auto f4 = r.Lock();
  EXPECT_FALSE(f4.IsReady());

  EXPECT_TRUE(r.UnLock());
  EXPECT_FALSE(f3.IsReady());

  EXPECT_TRUE(r.UnLock());
  EXPECT_TRUE(f3.IsReady());

  EXPECT_FALSE(f4.IsReady());

  EXPECT_TRUE(w.UnLock());
  EXPECT_TRUE(f4.IsReady());
}

/// @brief Test for double read lock.
TEST(RWLock, RWLockLock) {
  RWLock l;

  auto f1 = l.ReadLock();
  auto f2 = l.ReadLock();

  EXPECT_TRUE(f1.IsReady());
  EXPECT_FALSE(f1.IsFailed());

  EXPECT_TRUE(f2.IsReady());
  EXPECT_FALSE(f2.IsFailed());
}

/// @brief Test for try lock.
TEST(RWLock, RWLockTryLock) {
  RWLock l;
  EXPECT_TRUE(l.TryReadLock());
  EXPECT_FALSE(l.TryWriteLock());

  EXPECT_TRUE(l.TryReadLock());
  EXPECT_TRUE(l.ReadUnLock());

  EXPECT_FALSE(l.TryWriteLock());

  EXPECT_TRUE(l.ReadUnLock());
  EXPECT_TRUE(l.TryWriteLock());
}

}  // namespace trpc
