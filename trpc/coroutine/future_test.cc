//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/future_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/future.h"

#include <chrono>
#include <functional>
#include <thread>
#include <tuple>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/coroutine/fiber/runtime.h"
#include "trpc/util/latch.h"

namespace trpc::fiber::testing {

using namespace std::literals;

TEST(BlockingGet, BlockingGetWithPthreadTest) {
  int fiber_count{0};
  trpc::FiberLatch fiber_latch(1);
  trpc::StartFiberDetached([&] {
    trpc::Promise<std::string> promise;
    auto fut = promise.GetFuture();
    std::thread t([p = std::move(promise), &fiber_count]() mutable {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      fiber_count++;
      std::string msg = "ok";
      p.SetValue(std::move(msg));
    });
    t.detach();

    auto valid_future = trpc::fiber::BlockingGet(std::move(fut));
    ASSERT_EQ(valid_future.GetValue0(), "ok");

    fiber_latch.CountDown();
  });

  fiber_latch.Wait();

  ASSERT_EQ(fiber_count, 1);
}

TEST(BlockingTryGet, BlockingTryGetWithPthreadTest) {
  int fiber_count{0};
  trpc::FiberLatch fiber_latch(1);
  trpc::StartFiberDetached([&] {
    trpc::Promise<std::string> promise;
    auto fut = promise.GetFuture();
    std::thread t([p = std::move(promise), &fiber_count]() mutable {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      fiber_count++;
      std::string msg = "ok";
      p.SetValue(std::move(msg));
    });
    t.detach();

    auto valid_future = trpc::fiber::BlockingTryGet(std::move(fut), 4000);

    ASSERT_EQ(fiber_count, 1);
    ASSERT_EQ((*valid_future).GetValue0(), "ok");

    fiber_latch.CountDown();
  });

  fiber_latch.Wait();
}

TEST(BlockingTryGet, BlockingTryGetWithPthreadTestNoValue) {
  int fiber_count{0};
  trpc::FiberLatch fiber_latch(1);
  trpc::StartFiberDetached([&] {
    trpc::Promise<> promise;
    auto fut = promise.GetFuture();
    std::thread t([p = std::move(promise), &fiber_count]() mutable {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      fiber_count++;
      p.SetValue();
    });
    t.detach();

    auto valid_future = trpc::fiber::BlockingTryGet(std::move(fut), 4000);

    ASSERT_EQ(fiber_count, 1);
    ASSERT_TRUE((*valid_future).IsReady());

    fiber_latch.CountDown();
  });

  fiber_latch.Wait();
}

TEST(BlockingTryGet, BlockingTryGetWithPthreadTestMultiValue) {
  int fiber_count{0};
  trpc::FiberLatch fiber_latch(1);
  trpc::StartFiberDetached([&] {
    trpc::Promise<std::string, int> promise;
    auto fut = promise.GetFuture();
    std::thread t([p = std::move(promise), &fiber_count]() mutable {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      fiber_count++;
      p.SetValue(std::make_tuple("ok", 1));
    });
    t.detach();

    auto valid_future = trpc::fiber::BlockingTryGet(std::move(fut), 4000);

    ASSERT_EQ(fiber_count, 1);
    std::tuple<std::string, int> result = (*valid_future).GetValue();
    ASSERT_EQ(std::get<0>(result), "ok");
    ASSERT_EQ(std::get<1>(result), 1);

    fiber_latch.CountDown();
  });

  fiber_latch.Wait();
}

TEST(BlockingTryGet, BlockingTryGetWithPthreadTestTimeout) {
  int fiber_count{0};
  trpc::FiberLatch fiber_latch(1);
  trpc::StartFiberDetached([&] {
    trpc::Promise<std::string> promise;
    auto fut = promise.GetFuture();
    std::thread t([p = std::move(promise), &fiber_count]() mutable {
      std::this_thread::sleep_for(std::chrono::seconds(3));
      fiber_count++;
      std::string msg = "ok";
      p.SetValue(std::move(msg));
    });
    t.detach();

    auto invalid_future = trpc::fiber::BlockingTryGet(std::move(fut), 1000);

    ASSERT_EQ(fiber_count, 0);
    ASSERT_EQ(invalid_future, std::nullopt);

    fiber_latch.CountDown();
  });

  fiber_latch.Wait();
}

}  // namespace trpc::fiber::testing

int Start(int argc, char** argv, std::function<int(int, char**)> cb) {
  signal(SIGPIPE, SIG_IGN);

  trpc::fiber::StartRuntime();

  int rc = 0;
  {
    trpc::Latch l(1);
    trpc::StartFiberDetached([&] {
      rc = cb(argc, argv);

      l.count_down();
    });
    l.wait();
  }

  trpc::fiber::TerminateRuntime();

  return rc;
}

int InitAndRunAllTests(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return Start(argc, argv, [](auto, auto) { return ::RUN_ALL_TESTS(); });
}

int main(int argc, char** argv) { return InitAndRunAllTests(argc, argv); }
