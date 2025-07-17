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

#include "trpc/util/container/bounded_queue.h"

#include "gtest/gtest.h"

namespace {

using trpc::container::BoundedQueue;
using trpc::container::StorageOwnership;

}  // namespace

namespace trpc::container::testing {

TEST(BoundedQueueTest, sanity) {
  const int N = 36;
  char storage[N * sizeof(int)];
  BoundedQueue<int> q(storage, sizeof(storage), StorageOwnership::NOT_OWN_STORAGE);
  ASSERT_EQ(0ul, q.Size());
  ASSERT_TRUE(q.Empty());
  ASSERT_TRUE(q.Top() == nullptr);
  ASSERT_TRUE(q.Bottom() == nullptr);
  for (int i = 1; i <= N; ++i) {
    if (i % 2 == 0) {
      ASSERT_TRUE(q.Push(i));
    } else {
      int* p = q.Push();
      ASSERT_TRUE(p);
      *p = i;
    }
    ASSERT_EQ((size_t)i, q.Size());
    ASSERT_EQ(1, *q.Top());
    ASSERT_EQ(i, *q.Bottom());
  }
  ASSERT_FALSE(q.Push(N + 1));
  ASSERT_FALSE(q.PushTop(N + 1));
  ASSERT_EQ((size_t)N, q.Size());
  ASSERT_FALSE(q.Empty());
  ASSERT_TRUE(q.Full());

  for (int i = 1; i <= N; ++i) {
    ASSERT_EQ(i, *q.Top());
    ASSERT_EQ(N, *q.Bottom());
    if (i % 2 == 0) {
      int tmp = 0;
      ASSERT_TRUE(q.Pop(&tmp));
      ASSERT_EQ(tmp, i);
    } else {
      ASSERT_TRUE(q.Pop());
    }
    ASSERT_EQ((size_t)(N - i), q.Size());
  }
  ASSERT_EQ(0ul, q.Size());
  ASSERT_TRUE(q.Empty());
  ASSERT_FALSE(q.Full());
  ASSERT_FALSE(q.Pop());

  for (int i = 1; i <= N; ++i) {
    if (i % 2 == 0) {
      ASSERT_TRUE(q.PushTop(i));
    } else {
      int* p = q.PushTop();
      ASSERT_TRUE(p);
      *p = i;
    }
    ASSERT_EQ((size_t)i, q.Size());
    ASSERT_EQ(i, *q.Top());
    ASSERT_EQ(1, *q.Bottom());
  }
  ASSERT_FALSE(q.Push(N + 1));
  ASSERT_FALSE(q.PushTop(N + 1));
  ASSERT_EQ((size_t)N, q.Size());
  ASSERT_FALSE(q.Empty());
  ASSERT_TRUE(q.Full());

  for (int i = 1; i <= N; ++i) {
    ASSERT_EQ(N, *q.Top());
    ASSERT_EQ(i, *q.Bottom());
    if (i % 2 == 0) {
      int tmp = 0;
      ASSERT_TRUE(q.PopBottom(&tmp));
      ASSERT_EQ(tmp, i);
    } else {
      ASSERT_TRUE(q.PopBottom());
    }
    ASSERT_EQ((size_t)(N - i), q.Size());
  }
  ASSERT_EQ(0ul, q.Size());
  ASSERT_TRUE(q.Empty());
  ASSERT_FALSE(q.Full());
  ASSERT_FALSE(q.Pop());
}

}  // namespace trpc::container::testing
