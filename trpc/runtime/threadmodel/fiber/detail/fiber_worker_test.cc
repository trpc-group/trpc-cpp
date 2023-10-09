//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_worker_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/fiber_worker.h"

#include <sys/time.h>
#include <unistd.h>

#include <deque>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/fiber/detail/stack_allocator_impl.h"
#include "trpc/runtime/threadmodel/fiber/detail/testing.h"
#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"
#include "trpc/util/thread/cpu.h"

using namespace std::literals;

namespace trpc::fiber::detail {

constexpr std::string_view kSchedulingNames[] = {kSchedulingV1, kSchedulingV2};

void TestAffinity1(std::string_view scheduling_name) {
  for (int k = 0; k != 100; ++k) {
    std::atomic<std::size_t> executed{0};
    auto sg = std::make_unique<SchedulingGroup>(std::vector<unsigned>{1, 2, 3}, 16, scheduling_name);
    TimerWorker dummy(sg.get());
    sg->SetTimerWorker(&dummy);
    std::deque<FiberWorker> workers;

    for (int i = 0; i != 16; ++i) {
      workers.emplace_back(sg.get(), i).Start();
    }
    testing::StartFiberEntityInGroup(sg.get(), [&] {
      auto cpu = trpc::GetCurrentProcessorId();
      ASSERT_TRUE(1 <= cpu && cpu <= 3);
      ++executed;
    });
    sg->Stop();
    for (auto&& w : workers) {
      w.Join();
    }
    ASSERT_EQ(1, executed);
  }
}

TEST(FiberWorker, Affinity1OnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestAffinity1(name);
  }
}

void TestAffinity2(std::string_view scheduling_name) {
  for (int k = 0; k != 100; ++k) {
    std::atomic<std::size_t> executed{0};
    auto sg = std::make_unique<SchedulingGroup>(std::vector<unsigned>{1, 2, 3}, 3, scheduling_name);
    TimerWorker dummy(sg.get());
    sg->SetTimerWorker(&dummy);
    std::deque<FiberWorker> workers;

    for (int i = 0; i != 3; ++i) {
      workers.emplace_back(sg.get(), i, true).Start();
    }
    testing::StartFiberEntityInGroup(sg.get(), [&] {
      auto cpu = trpc::GetCurrentProcessorId();
      ASSERT_TRUE(1 <= cpu && cpu <= 3);
      ++executed;
    });
    sg->Stop();
    for (auto&& w : workers) {
      w.Join();
    }
    ASSERT_EQ(1, executed);
  }
}

TEST(FiberWorker, Affinity2OnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestAffinity2(name);
  }
}

// Cross-group stealing is currently only supported by scheduling v1.
TEST(FiberWorker, StealFiberOnSchedulingV1) {
  std::atomic<std::size_t> executed{0};
  auto sg = std::make_unique<SchedulingGroup>(std::vector<unsigned>{1, 2, 3}, 16, kSchedulingV1);
  auto sg2 = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 1, kSchedulingV1);
  TimerWorker dummy(sg.get());
  sg->SetTimerWorker(&dummy);
  std::deque<FiberWorker> workers;

  testing::StartFiberEntityInGroup(sg2.get(), [&] { ++executed; });
  for (int i = 0; i != 16; ++i) {
    auto&& w = workers.emplace_back(sg.get(), i);
    w.AddForeignSchedulingGroup(sg2.get(), 1);
    w.Start();
  }
  testing::StartFiberEntityInGroup(sg.get(), [] {});  // To wake worker up.
  while (!executed) {
    std::this_thread::sleep_for(1ms);
  }
  sg->Stop();
  for (auto&& w : workers) {
    w.Join();
  }

  ASSERT_EQ(1, executed);
}

void TestTorture(std::string_view scheduling_name) {
  constexpr auto T = 32;
  // Setting it too large cause `vm.max_map_count` overrun.
  constexpr auto N = 32768;
  constexpr auto P = 128;
  SetFiberStackEnableGuardPage(false);
  for (int i = 0; i != 50; ++i) {
    std::atomic<std::size_t> executed{0};
    auto sg = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, T, scheduling_name);
    TimerWorker dummy(sg.get());
    sg->SetTimerWorker(&dummy);
    std::deque<FiberWorker> workers;

    for (int i = 0; i != T; ++i) {
      workers.emplace_back(sg.get(), i).Start();
    }

    int size = 0;
    // Concurrently create fibers.
    std::thread prods[P];
    for (auto&& t : prods) {
      t = std::thread([&] {
        constexpr auto kChildren = 32;
        static_assert(N % P == 0 && (N / P) % kChildren == 0);
        for (int i = 0; i != N / P / kChildren; ++i) {
          testing::StartFiberEntityInGroup(sg.get(), [&] {
            ++executed;
            for (auto j = 0; j != kChildren - 1; ++j) {
              testing::StartFiberEntityInGroup(sg.get(), [&] { ++executed; });
            }
          });
        }
      });
      ++size;
    }

    for (auto&& t : prods) {
      t.join();
    }

    while (executed != N) {
      std::this_thread::sleep_for(100ms);
    }
    sg->Stop();
    for (auto&& w : workers) {
      w.Join();
    }

    ASSERT_EQ(N, executed);
  }
}

TEST(FiberWorker, TortureOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestTorture(name);
  }
}

}  // namespace trpc::fiber::detail
