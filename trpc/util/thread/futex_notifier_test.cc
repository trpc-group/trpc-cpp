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

#include "trpc/util/thread/futex_notifier.h"

#include <functional>
#include <thread>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(FutexNotifierTest, FutexNotifierTest) {
  trpc::FutexNotifier futex_notifier;

  bool is_executed = false;
  std::thread t([&futex_notifier, &is_executed] {
    while (true) {
      const trpc::FutexNotifier::State st = futex_notifier.GetState();
      if (st.Stopped()) {
        break;
      } else {
        futex_notifier.Wait(st);
        is_executed = true;
      }
    }
  });

  while (!is_executed) {
    futex_notifier.Wake(1);
  }

  futex_notifier.Stop();

  t.join();
 
  ASSERT_TRUE(is_executed);
}

TEST(FutexNotifierTest, ReturnByETIMEDOUT) {
  trpc::FutexNotifier futex_notifier;
  bool is_awaken = true;
  const trpc::FutexNotifier::State st = futex_notifier.GetState();

  std::thread t([&futex_notifier, &is_awaken, &st] {
    // Set timeout to 1ms.
    std::chrono::steady_clock::time_point abs_time = ReadSteadyClock() + std::chrono::microseconds(1000);
    is_awaken = futex_notifier.Wait(st, &abs_time);
  });

  // Trigger timeout, no need to wake.
  t.join();
  ASSERT_FALSE(is_awaken);
}

TEST(FutexNotifierTest, ReturnByEAGAIN) {
  trpc::FutexNotifier futex_notifier;
  bool is_awaken = false;
  const trpc::FutexNotifier::State st = futex_notifier.GetState();

  std::thread t([&futex_notifier, &is_awaken, &st] {
    // Let wake first.
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    // Set timeout to 1ms.
    std::chrono::steady_clock::time_point abs_time = ReadSteadyClock() + std::chrono::microseconds(1000);
    // Wait after wake, equal to awakened up.
    is_awaken = futex_notifier.Wait(st, &abs_time);
  });

  futex_notifier.Wake(1);

  t.join();
  ASSERT_TRUE(is_awaken);
}

TEST(FutexNotifierTest, ReturnByWakeWithoutTimeoutSet) {
  trpc::FutexNotifier futex_notifier;
  bool is_awaken = false;
  const trpc::FutexNotifier::State st = futex_notifier.GetState();

  std::thread t([&futex_notifier, &is_awaken, &st] {
    is_awaken = futex_notifier.Wait(st, NULL);
  });

  // Let wait first.
  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  futex_notifier.Wake(1);

  t.join();
  ASSERT_TRUE(is_awaken);
}

TEST(FutexNotifierTest, ReturnByWakeWithTimeoutSet) {
  trpc::FutexNotifier futex_notifier;
  bool is_awaken = false;
  const trpc::FutexNotifier::State st = futex_notifier.GetState();

  std::thread t([&futex_notifier, &is_awaken, &st] {
    // Set timeout to large enough 10s.
    std::chrono::steady_clock::time_point abs_time = ReadSteadyClock() + std::chrono::microseconds(10000000);
    is_awaken = futex_notifier.Wait(st, &abs_time);
  });

  // Let wait first.
  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  futex_notifier.Wake(1);

  t.join();
  ASSERT_TRUE(is_awaken);
}

TEST(FutexNotifierTest, TimeoutLessThanNow) {
  trpc::FutexNotifier futex_notifier;
  bool is_awaken = true;
  const trpc::FutexNotifier::State st = futex_notifier.GetState();

  std::thread t([&futex_notifier, &is_awaken, &st] {
    // Set timeout to less than now.
    std::chrono::steady_clock::time_point abs_time = ReadSteadyClock() - std::chrono::microseconds(1000);
    is_awaken = futex_notifier.Wait(st, &abs_time);
  });

  t.join();
  ASSERT_FALSE(is_awaken);
}

}  // namespace trpc::testing
