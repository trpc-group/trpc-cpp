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

#include "trpc/coroutine/fiber_blocking_bounded_queue.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/time.h"

namespace trpc::testing {

TEST(FiberBlockingBoundedQueueTest, Run) {
  RunAsFiber([&]() {
    FiberBlockingBoundedQueue<int> queue{0};
    ASSERT_EQ(0, queue.Capacity());
    size_t capacity = 2;
    queue.SetCapacity(capacity);
    ASSERT_EQ(capacity, queue.Capacity());

    ASSERT_EQ(0, queue.Size());
    int out;
    ASSERT_FALSE(queue.TryPop(out));
    ASSERT_FALSE(queue.Pop(out, std::chrono::milliseconds(1)));

    ASSERT_TRUE(queue.TryPush(1));
    ASSERT_EQ(1, queue.Size());
    ASSERT_TRUE(queue.TryPop(out));
    ASSERT_EQ(1, out);
    ASSERT_EQ(0, queue.Size());

    queue.Push(2);
    ASSERT_EQ(1, queue.Size());
    queue.Pop(out);
    ASSERT_EQ(2, out);
    ASSERT_EQ(0, queue.Size());
  });
}

}  // namespace trpc::testing
