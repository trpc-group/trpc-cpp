//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/execution_context_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_execution_context.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/fiber_timer.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc {

TEST(FiberExecutionContext, NullContext) {
  RunAsFiber([] { ASSERT_EQ(nullptr, FiberExecutionContext::Current()); });
}

TEST(FiberExecutionContext, RunInContext) {
  RunAsFiber([] {
    ASSERT_EQ(nullptr, FiberExecutionContext::Current());
    auto ctx = FiberExecutionContext::Create();
    static FiberExecutionLocal<int> els_int;

    ctx->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx.Get());
      *els_int = 10;
      ASSERT_EQ(10, *els_int);
    });
    ctx->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx.Get());
      ASSERT_EQ(10, *els_int);
    });

    auto ctx2 = FiberExecutionContext::Create();
    ctx2->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx2.Get());
      *els_int = 5;
      ASSERT_EQ(5, *els_int);
    });
    ctx->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx.Get());
      ASSERT_EQ(10, *els_int);
      auto capture_ctx = FiberExecutionContext::Capture();
      ASSERT_EQ(capture_ctx.Get(), ctx.Get());
      ASSERT_EQ(capture_ctx.UseCount(), ctx.UseCount());
    });
    ctx2->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx2.Get());
      ASSERT_EQ(5, *els_int);
    });
  });
}

TEST(FiberExecutionContext, TimerPropagation) {
  RunAsFiber([] {
    ASSERT_EQ(nullptr, FiberExecutionContext::Current());
    auto ctx = FiberExecutionContext::Create();
    static FiberExecutionLocal<int> els_int;

    ctx->Execute([&] {
      *els_int = 10;
      FiberLatch latch(2);
      SetFiberDetachedTimer(ReadSteadyClock() + std::chrono::milliseconds(100), [&] {
        ASSERT_EQ(10, *els_int);
        latch.CountDown();
      });
      SetFiberDetachedTimer(ReadSteadyClock() + std::chrono::milliseconds(50), [&] {
        ASSERT_EQ(10, *els_int);
        latch.CountDown();
      });
      latch.Wait();
    });
  });
}

TEST(FiberExecutionLocal, All) {
  RunAsFiber([] {
    ASSERT_EQ(nullptr, FiberExecutionContext::Current());
    auto ctx = FiberExecutionContext::Create();
    static FiberExecutionLocal<int> els_int;
    static FiberExecutionLocal<int> els_int2;
    static FiberExecutionLocal<double> els_dbl;

    ctx->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx.Get());
      *els_int = 10;
      *els_int2 = 11;
      *els_dbl = 12;
    });
    ctx->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx.Get());
      ASSERT_EQ(10, *els_int);
      ASSERT_EQ(11, *els_int2);
      ASSERT_EQ(12, *els_dbl);
    });

    auto ctx2 = FiberExecutionContext::Create();
    ctx2->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx2.Get());
      *els_int = 0;
      *els_int2 = 1;
      *els_dbl = 2;
    });
    ctx2->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx2.Get());
      ASSERT_EQ(0, *els_int);
      ASSERT_EQ(1, *els_int2);
      ASSERT_EQ(2, *els_dbl);
    });
    ctx->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx.Get());
      ASSERT_EQ(10, *els_int);
      ASSERT_EQ(11, *els_int2);
      ASSERT_EQ(12, *els_dbl);
    });
    ctx2->Execute([&] {
      ASSERT_EQ(FiberExecutionContext::Current(), ctx2.Get());
      ASSERT_EQ(0, *els_int);
      ASSERT_EQ(1, *els_int2);
      ASSERT_EQ(2, *els_dbl);
    });
  });
}

}  // namespace trpc
