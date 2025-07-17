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

#include "trpc/coroutine/fiber_blocking_noncontiguous_buffer.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/time.h"

namespace trpc::testing {

TEST(FiberBlockingBoundedQueueTest, Run) {
  RunAsFiber([&]() {
    FiberBlockingNoncontiguousBuffer queue{0};
    ASSERT_EQ(0, queue.Capacity());
    size_t capacity = 2;
    queue.SetCapacity(capacity);
    ASSERT_EQ(capacity, queue.Capacity());

    //  Attempting to read from the queue when there is no data.
    ASSERT_EQ(0, queue.ByteSize());
    NoncontiguousBuffer out;
    ASSERT_FALSE(queue.TryCut(out, 1));
    ASSERT_FALSE(queue.Cut(out, 1, std::chrono::milliseconds(1)));

    char* str = new char[2];
    str[0] = '1';
    str[1] = '\0';
    out.Append(str, 1);
    ASSERT_TRUE(queue.TryAppend(std::move(out)));
    ASSERT_EQ(1, queue.ByteSize());
    ASSERT_TRUE(queue.TryCut(out, 1));
    ASSERT_EQ(1, out.ByteSize());
    ASSERT_EQ(0, queue.ByteSize());

    out.Clear();
    str = new char[3];
    str[0] = '1';
    str[1] = '2';
    str[2] = '\0';
    out.Append(str, 2);
    queue.Append(std::move(out));
    ASSERT_EQ(2, queue.ByteSize());
    queue.Cut(out, 2);
    ASSERT_EQ(2, out.ByteSize());
    ASSERT_EQ(0, queue.ByteSize());
  });
}

TEST(FiberBlockingBoundedQueueTest, Stop) {
  RunAsFiber([&]() {
    FiberBlockingNoncontiguousBuffer queue{0};

    trpc::StartFiberDetached([&]() {
      trpc::FiberSleepFor(std::chrono::seconds(1));
      queue.Stop();
    });

    ASSERT_EQ(0, queue.Capacity());
    NoncontiguousBuffer out;
    ASSERT_TRUE(queue.Cut(out, std::numeric_limits<size_t>::max(), std::chrono::seconds(3)));
  });
}

}  // namespace trpc::testing
