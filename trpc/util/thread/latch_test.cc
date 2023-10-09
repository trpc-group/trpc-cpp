//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/latch_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/latch.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/util/chrono/chrono.h"

namespace trpc::testing {

std::atomic<bool> exiting{false};

void RunTest() {
  std::size_t local_count = 0, remote_count = 0;
  while (!exiting) {
    auto called = std::make_shared<std::atomic<bool>>(false);
    std::this_thread::yield();  // Wait for thread pool to start.
    Latch l(1);
    auto t = std::thread([&] {
      if (!called->exchange(true)) {
        std::this_thread::yield();  // Something costly.
        l.count_down();
        ++remote_count;
      }
    });
    std::this_thread::yield();  // Something costly.
    if (!called->exchange(true)) {
      l.count_down();
      ++local_count;
    }
    l.wait();
    t.join();
  }
  std::cout << local_count << " " << remote_count << std::endl;
}

TEST(Latch, Torture) {
  std::thread ts[10];
  for (auto&& t : ts) {
    t = std::thread(RunTest);
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  exiting = true;
  for (auto&& t : ts) {
    t.join();
  }
}

TEST(Latch, CountDownTwo) {
  Latch l(2);
  ASSERT_FALSE(l.try_wait());
  l.arrive_and_wait(2);
  ASSERT_TRUE(1);
  ASSERT_TRUE(l.try_wait());
}

TEST(Latch, WaitFor) {
  Latch l(1);
  ASSERT_FALSE(l.wait_for(std::chrono::milliseconds(100)));
  l.count_down();
  ASSERT_TRUE(l.wait_for(std::chrono::milliseconds(0)));
}

TEST(Latch, WaitUntil) {
  Latch l(1);
  ASSERT_FALSE(l.wait_until(ReadSteadyClock() + std::chrono::milliseconds(100)));
  l.count_down();
  ASSERT_TRUE(l.wait_until(ReadSteadyClock()));
}

}  // namespace trpc
