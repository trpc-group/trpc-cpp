//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/time_keeper_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/time_keeper.h"

#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/chrono/chrono.h"

using namespace std::literals;

namespace trpc::internal::testing {

TEST(TimeKeeper, AddTimer) {
  TimeKeeper::Instance()->Start();
  std::thread t1([] {
    int x = 0;
    auto timer_id = TimeKeeper::Instance()->AddTimer(
        ReadSteadyClock(), 10ms, [&](auto) { ++x; }, false);
    std::this_thread::sleep_for(1s);
    TimeKeeper::Instance()->KillTimer(timer_id);
    ASSERT_NEAR(x, 100, 100);

    std::vector<std::uint64_t> timers;
    std::atomic<int> y{};
    for (int i = 0; i != 1000; ++i) {
      timers.push_back(TimeKeeper::Instance()->AddTimer(
          ReadSteadyClock(), 10ms, [&](auto) { ++y; }, true));
    }
    std::this_thread::sleep_for(1s);
    for (auto&& e : timers) {
      TimeKeeper::Instance()->KillTimer(e);
    }
    ASSERT_NEAR(y, 1000 * 100, 1000 * 100);
  });

  t1.join();

  TimeKeeper::Instance()->Stop();
  TimeKeeper::Instance()->Join();
}

// Test whether can stop normally when the task queue is empty
TEST(TimeKeeper, TestStop) {
  TimeKeeper::Instance()->Start();

  sleep(1);

  TimeKeeper::Instance()->Stop();
  TimeKeeper::Instance()->Join();
}

}  // namespace trpc::internal::testing
