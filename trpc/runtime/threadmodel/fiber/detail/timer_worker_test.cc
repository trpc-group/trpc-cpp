//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/timer_worker_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"

#include <thread>

#include "gtest/gtest.h"

#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/latch.h"
#include "trpc/util/random.h"

using namespace std::literals;

namespace trpc::fiber::detail {

namespace {

template <class SG, class... Args>
[[nodiscard]] std::uint64_t SetTimerAt(SG&& sg, Args&&... args) {
  auto tid = sg->CreateTimer(std::forward<Args>(args)...);
  sg->EnableTimer(tid);
  return tid;
}

}  // namespace

constexpr std::string_view kSchedulingNames[] = {kSchedulingV1, kSchedulingV2};

void TestEarlyTimer(std::string_view scheduling_name) {
  std::atomic<bool> called = false;

  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{1, 2, 3}, 1, scheduling_name);
  TimerWorker worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&worker);
  std::thread t = std::thread([&scheduling_group, &called] {
    scheduling_group->EnterGroup(0);

    Latch l(1);
    (void)SetTimerAt(scheduling_group, std::chrono::steady_clock::time_point::min(), [&](auto tid) {
      scheduling_group->RemoveTimer(tid);
      called = true;
      l.count_down();
    });
    l.wait();

    scheduling_group->LeaveGroup();
  });

  worker.Start();
  t.join();
  worker.Stop();
  worker.Join();

  ASSERT_TRUE(called);
}

TEST(TimerWorker, EarlyTimerOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestEarlyTimer(name);
  }
}

void TestSetTimerInTimer(std::string_view scheduling_name) {
  std::atomic<bool> called = false;

  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{1, 2, 3}, 1, scheduling_name);
  TimerWorker worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&worker);
  std::thread t = std::thread([&scheduling_group, &called] {
    scheduling_group->EnterGroup(0);

    auto timer_cb = [&](auto tid) {
      auto timer_cb2 = [&, tid](auto tid2) {
        scheduling_group->RemoveTimer(tid);
        scheduling_group->RemoveTimer(tid2);
        called = true;
      };
      (void)SetTimerAt(scheduling_group, std::chrono::steady_clock::time_point{}, timer_cb2);
    };
    (void)SetTimerAt(scheduling_group, std::chrono::steady_clock::time_point::min(), timer_cb);
    std::this_thread::sleep_for(100ms);
    scheduling_group->LeaveGroup();
  });

  worker.Start();

  while (!called) {
    usleep(100000);
  }

  t.join();
  worker.Stop();
  worker.Join();

  ASSERT_TRUE(called);
}

TEST(TimerWorker, SetTimerInTimerContextOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestSetTimerInTimer(name);
  }
}

std::atomic<std::size_t> timer_set, timer_removed;

void TestTorture(std::string_view scheduling_name) {
  constexpr auto N = 1000;
  constexpr auto T = 16;

  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{1, 2, 3}, T, scheduling_name);
  TimerWorker worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&worker);
  std::thread ts[T];

  for (int i = 0; i != T; ++i) {
    ts[i] = std::thread([i, &scheduling_group] {
      scheduling_group->EnterGroup(i);

      for (int j = 0; j != N; ++j) {
        auto timeout = ReadSteadyClock() + Random(2'000'000) * 1us;
        if (j % 2 == 0) {
          // In this case we set a timer and let it fire.
          (void)SetTimerAt(scheduling_group, timeout, [&scheduling_group](auto timer_id) {
            scheduling_group->RemoveTimer(timer_id);
            ++timer_removed;
          });  // Indirectly calls `TimerWorker::AddTimer`.
          ++timer_set;
        } else {
          // In this case we set a timer and cancel it sometime later.
          auto timer_id = SetTimerAt(scheduling_group, timeout, [](auto) {});
          (void)SetTimerAt(scheduling_group, ReadSteadyClock() + Random(1000) * 1ms,
                           [timer_id, &scheduling_group](auto self) {
                             scheduling_group->RemoveTimer(timer_id);
                             scheduling_group->RemoveTimer(self);
                             ++timer_removed;
                           });
          ++timer_set;
        }
        if (j % 10000 == 0) {
          std::this_thread::sleep_for(100ms);
        }
      }

      // Wait until all timers have been consumed. Otherwise if we leave the
      // thread too early, `TimerWorker` might incurs use-after-free when
      // accessing our thread-local timer queue.
      while (timer_removed.load(std::memory_order_relaxed) !=
             timer_set.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(100ms);
      }
      scheduling_group->LeaveGroup();
    });
  }

  worker.Start();

  for (auto&& t : ts) {
    t.join();
  }
  worker.Stop();
  worker.Join();

  ASSERT_EQ(timer_set, timer_removed);
  ASSERT_EQ(N * T, timer_set);
}

TEST(TimerWorker, TortureOnSchedulingV2) {
  TestTorture(kSchedulingV2);
}

}  // namespace trpc::fiber::detail
