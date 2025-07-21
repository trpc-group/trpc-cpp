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

#include "trpc/coroutine/testing/fiber_runtime.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_event.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/util/time.h"

using namespace std::literals;

namespace trpc::testing {

void TestFiberLatch(size_t prev_begin) {
  size_t n = 100;
  FiberLatch l(1);
  std::atomic<uint32_t> counter = 0;
  size_t begin = trpc::time::GetNanoSeconds();
  for (size_t i = 0; i < n; ++i) {
    StartFiberDetached([&]{
      if (counter.fetch_add(1) == (n - 1)) {
        l.CountDown();
      }
    });
  }
  l.Wait();
  size_t end = trpc::time::GetNanoSeconds();
  std::cout << "fiber_latch timecost(ns):" << (end - begin) << ", wait:" << (end - prev_begin) << std::endl;
}

void TestFiberEvent(size_t prev_begin) {
  size_t n = 100;
  FiberEvent e;
  std::atomic<uint32_t> counter = 0;
  size_t begin = trpc::time::GetNanoSeconds();
  for (size_t i = 0; i < n; ++i) {
    StartFiberDetached([&]{
      if (counter.fetch_add(1) == (n - 1)) {
        e.Set();
      }
    });
  }
  e.Wait();
  size_t end = trpc::time::GetNanoSeconds();
  std::cout << "fiber_event timecost(ns):" << (end - begin) << ", wait:" << (end - prev_begin) << std::endl;
}

TEST(RunAsFiber, TestRunFiberLatch) {
  testing::RunAsFiber([] {
    size_t n = 20;
    FiberLatch l(1);
    std::atomic<uint32_t> counter = 0;
    size_t begin = trpc::time::GetNanoSeconds();
    for (size_t i = 0; i != n; ++i) {
      StartFiberDetached([&]{
        TestFiberLatch(begin);
        if (counter.fetch_add(1) == (n - 1)) {
          l.CountDown();
        }
      });
    }
    l.Wait();
    size_t end = trpc::time::GetNanoSeconds();
    std::cout << "total timecost(ns):" << (end - begin) << std::endl;
    ASSERT_TRUE(1);
  });
}

TEST(RunAsFiber, TestRunFiberEvent) {
  testing::RunAsFiber([] {
    size_t n = 20;
    FiberEvent e;
    std::atomic<uint32_t> counter = 0;
    size_t begin = trpc::time::GetNanoSeconds();
    for (size_t i = 0; i != n; ++i) {
      StartFiberDetached([&]{
        TestFiberLatch(begin);
        if (counter.fetch_add(1) == (n - 1)) {
          e.Set();
        }
      });
    }
    e.Wait();
    size_t end = trpc::time::GetNanoSeconds();
    std::cout << "total timecost(ns):" << (end - begin) << std::endl;
    ASSERT_TRUE(1);
  });
}

}  // namespace trpc::testing
