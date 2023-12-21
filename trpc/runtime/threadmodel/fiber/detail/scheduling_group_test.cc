//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/scheduling_group_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/testing.h"
#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"
#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/random.h"
#include "trpc/util/string_helper.h"

using namespace std::literals;

namespace trpc::fiber::detail {

namespace {

template <class SG, class... Args>
[[nodiscard]] std::uint64_t SetTimerAt(SG&& sg, Args&&... args) {
  auto tid = sg->CreateTimer(std::forward<Args>(args)...);
  sg->EnableTimer(tid);
  return tid;
}

std::size_t GetMaxFibers() {
  std::ifstream ifs("/proc/sys/vm/max_map_count");
  std::string s;
  std::getline(ifs, s);
  return std::min<std::size_t>(TryParse<std::size_t>(s).value_or(65530) / 2, 1048576);
}

}  // namespace

constexpr std::string_view kSchedulingNames[] = {kSchedulingV1, kSchedulingV2};

void TestCreate(std::string_view scheduling_name) {
  // Hopefully we have at least 4 cores on machine running UT.
  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{0, 1, 2, 3}, 20, scheduling_name);

  auto scheduling_group2 = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 20, scheduling_name);
}

TEST(SchedulingGroup, CreateOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestCreate(name);
  }
}

void WorkerTest(SchedulingGroup* sg, std::size_t index) {
  sg->EnterGroup(index);
  sg->Schedule();
  sg->LeaveGroup();
}

struct Context {
  std::atomic<std::size_t> executed_fibers{0};
  std::atomic<std::size_t> yields{0};
};

void FiberProc(Context* ctx) {
  auto sg = SchedulingGroup::Current();
  auto self = GetCurrentFiberEntity();
  std::atomic<std::size_t> yield_count_local{0};

  // It can't be.
  ASSERT_NE(self, GetMasterFiberEntity());

  for (int i = 0; i != 10; ++i) {
    ASSERT_EQ(FiberState::Running, self->state);
    sg->Yield(self);
    ++ctx->yields;
    ASSERT_EQ(self, GetCurrentFiberEntity());
    ++yield_count_local;
  }

  // The fiber is resumed by two worker concurrently otherwise.
  ASSERT_EQ(10, yield_count_local);

  ++ctx->executed_fibers;
}

void TestRunFibers(std::string_view scheduling_name) {
  static const auto N = GetMaxFibers();
  TRPC_FMT_INFO("Starting {} fibers.", N);

  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 16, scheduling_name);
  std::thread workers[16];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  for (int i = 0; i != 16; ++i) {
    workers[i] = std::thread(WorkerTest, scheduling_group.get(), i);
  }

  Context context;

  for (size_t i = 0; i != N; ++i) {
    // Even if we never call `FreeFiberEntity`, no leak should be reported (it's
    // implicitly recycled when `FiberProc` returns).
    fiber::testing::StartFiberEntityInGroup(scheduling_group.get(), [&] { FiberProc(&context); });
  }
  while (context.executed_fibers != N) {
    std::this_thread::sleep_for(100ms);
  }
  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(N, context.executed_fibers);
  ASSERT_EQ(N * 10, context.yields);
}

TEST(SchedulingGroup, RunFibersOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestRunFibers(name);
  }
}

void SwitchToNewFiber(SchedulingGroup* sg, std::size_t left, std::atomic<std::size_t>& switched) {
  if (--left) {
    auto next = CreateFiberEntity(sg, [sg, left, &switched] { SwitchToNewFiber(sg, left, switched); });
    sg->SwitchTo(GetCurrentFiberEntity(), next);
  }
  ++switched;
}

void TestSwitchToFiber(std::string_view scheduling_name) {
  constexpr auto N = 16384;

  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 16, scheduling_name);
  std::thread workers[16];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  for (int i = 0; i != 16; ++i) {
    workers[i] = std::thread(WorkerTest, scheduling_group.get(), i);
  }

  std::atomic<std::size_t> switched{};

  fiber::testing::StartFiberEntityInGroup(scheduling_group.get(), [&] {
    SwitchToNewFiber(scheduling_group.get(), N, switched);
  });
  while (switched != N) {
    std::this_thread::sleep_for(100ms);
  }
  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(N, switched);
}

TEST(SchedulingGroup, SwitchToFiberOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestSwitchToFiber(name);
  }
}

void TestWaitForFiberExit(std::string_view scheduling_name) {
  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 16, scheduling_name);
  std::thread workers[16];
  TimerWorker timer_worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&timer_worker);

  for (int i = 0; i != 16; ++i) {
    workers[i] = std::thread(WorkerTest, scheduling_group.get(), i);
  }
  timer_worker.Start();

  for (int k = 0; k != 100; ++k) {
    constexpr auto N = 1024;
    std::atomic<std::size_t> waited{};
    for (int i = 0; i != N; ++i) {
      auto f1 = CreateFiberEntity(scheduling_group.get(), [] {
        WaitableTimer wt(ReadSteadyClock() + Random(10) * 1ms);
        wt.wait();
      });
      f1->exit_barrier = object_pool::MakeLwShared<ExitBarrier>();
      auto f2 =
          CreateFiberEntity(scheduling_group.get(), [&, wc = f1->exit_barrier] {
            wc->Wait();
            ++waited;
          });
      if (Random() % 2 == 0) {
        scheduling_group->Resume(f1);
        scheduling_group->Resume(f2);
      } else {
        scheduling_group->Resume(f2);
        scheduling_group->Resume(f1);
      }
    }
    while (waited != N) {
      std::this_thread::sleep_for(10ms);
    }
  }
  scheduling_group->Stop();
  timer_worker.Stop();
  timer_worker.Join();
  for (auto&& t : workers) {
    t.join();
  }
}

TEST(SchedulingGroup, WaitForFiberExitOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestWaitForFiberExit(name);
  }
}

void SleepyFiberProc(std::atomic<std::size_t>* leaving) {
  auto self = GetCurrentFiberEntity();
  auto sg = self->scheduling_group;
  std::atomic<std::size_t> timer_set{};
  std::atomic<std::size_t> timer_removed{};

  std::unique_lock lk(self->scheduler_lock);

  auto timer_cb = [self, &timer_removed](auto) {
    ++timer_removed;
    self->scheduling_group->Resume(self);
  };
  auto timer_id =
      SetTimerAt(sg, ReadSteadyClock() + 1s + Random(1000'000) * 1us, timer_cb);
  ++timer_set;

  sg->Suspend(self, std::move(lk));
  sg->RemoveTimer(timer_id);
  ++*leaving;
}

void TestSetTimer(std::string_view scheduling_name) {
  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 16, scheduling_name);
  std::thread workers[16];
  std::atomic<std::size_t> leaving{0};
  TimerWorker timer_worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&timer_worker);

  for (int i = 0; i != 16; ++i) {
    workers[i] = std::thread(WorkerTest, scheduling_group.get(), i);
  }
  timer_worker.Start();

  constexpr auto N = 10;
  for (int i = 0; i != N; ++i) {
    fiber::testing::StartFiberEntityInGroup(scheduling_group.get(), [&] { SleepyFiberProc(&leaving); });
  }
  while (leaving != N) {
    std::this_thread::sleep_for(100ms);
  }
  scheduling_group->Stop();
  timer_worker.Stop();
  timer_worker.Join();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(N, leaving);
}

TEST(SchedulingGroup, SetTimerOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestSetTimer(name);
  }
}

void TestSetTimerPeriodic(std::string_view scheduling_name) {
  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 1, scheduling_name);
  TimerWorker timer_worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&timer_worker);
  auto worker = std::thread(WorkerTest, scheduling_group.get(), 0);
  timer_worker.Start();

  auto start = ReadSteadyClock();
  std::atomic<std::size_t> called{};
  std::atomic<std::uint64_t> timer_id;
  fiber::testing::StartFiberEntityInGroup(scheduling_group.get(), [&] {
    auto cb = [&](auto) {
      if (called < 10) {
        ++called;
      }
    };
    timer_id = SetTimerAt(scheduling_group, ReadSteadyClock() + 20ms, 100ms, cb);
  });
  while (called != 10) {
    std::this_thread::sleep_for(1ms);
  }
  scheduling_group->RemoveTimer(timer_id);
  scheduling_group->Stop();
  timer_worker.Stop();
  timer_worker.Join();
  worker.join();
  ASSERT_EQ(10, called);
  ASSERT_GE((ReadSteadyClock() - start) / 1ms, 1);
}

TEST(SchedulingGroup, SetTimerPeriodicOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestSetTimerPeriodic(name);
  }
}

TEST(SchedulingGroup, StartFibers) {
  RunAsFiber([&] {
    static constexpr auto N = 2;
    std::atomic<std::size_t> done{};
    std::vector<Function<void()>> procs;
    for (int j = 0; j != N; ++j) {
      procs.push_back([&] { ++done; });
    }

    BatchStartFiberDetached(std::move(procs));

    std::this_thread::sleep_for(500ms);

    ASSERT_GE(N, done.load());
  });
}

void MockWorkerTestAndStartFiber(SchedulingGroup* sg, std::size_t index, Latch* l) {
  sg->EnterGroup(index);

  // Simulate a scenario where the number of fibers exceeds the limit
  // const auto B = GetMaxFibers() + 100;
  const auto B = GetFiberRunQueueSize() + 10;
  TRPC_FMT_INFO("StartFiberDetached Starting too many fiber entity {}.", B);
  std::atomic<std::size_t> too_many_fiber_entity_done{};
  for (size_t i = 0; i != B; ++i) {
    StartFiberDetached([&] { ++too_many_fiber_entity_done; });
  }

  std::atomic<std::size_t> start_with_attr_fiber_entity_done{};
  Fiber::Attributes attr;
  StartFiberDetached(std::move(attr), [&] { ++start_with_attr_fiber_entity_done; });
  TRPC_LOG_INFO("StartFiberDetachedWithAttr Starting too many fiber entity");

  // Simulate a scenario where fibers are piling up without consuming any of them.
  while (true) {
    std::this_thread::sleep_for(1000ms);
    break;
  }

  ASSERT_GE(B, too_many_fiber_entity_done.load());
  ASSERT_GE(1, start_with_attr_fiber_entity_done.load());

  sg->LeaveGroup();

  l->count_down();
}

void TestCreateTooManyFiberEntity(std::string_view scheduling_name) {
  constexpr size_t kFiberWorkerSize = 1;
  SetFiberRunQueueSize(128);

  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, kFiberWorkerSize,
                                                            scheduling_name);
  std::thread workers[kFiberWorkerSize];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  Latch l(kFiberWorkerSize);
  for (size_t i = 0; i != kFiberWorkerSize; ++i) {
    workers[i] = std::thread(MockWorkerTestAndStartFiber, scheduling_group.get(), i, &l);
  }
  l.wait();

  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
}

TEST(SchedulingGroup, CreateTooManyFiberEntityOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestCreateTooManyFiberEntity(name);
  }
}

// Test that executing FiberYield under a single fiber worker does not cause thread spinning and lead to a deadlock
void TestFiberYield(std::string_view scheduling_name) {
  constexpr size_t kFiberWorkerSize = 1;
  auto scheduling_group = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, kFiberWorkerSize, scheduling_name);
  std::thread workers[kFiberWorkerSize];
  TimerWorker timer_worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&timer_worker);

  for (int i = 0; i != kFiberWorkerSize; ++i) {
    workers[i] = std::thread(WorkerTest, scheduling_group.get(), i);
  }
  timer_worker.Start();

  std::atomic<bool> is_ready{false};
  std::atomic<bool> is_done{false};
  fiber::testing::StartFiberEntityInGroup(scheduling_group.get(), [&is_ready, &is_done] {
    while (!is_ready) {
      FiberYield();
    }
    is_done = true;
  });

  fiber::testing::StartFiberEntityInGroup(scheduling_group.get(), [&is_ready] { is_ready = true; });

  // Wait for the fiber task to complete, waiting for a maximum of 2 seconds
  int loop = 0;
  while (!is_done && loop < 20) {
    std::this_thread::sleep_for(100ms);
    loop++;
  }
  ASSERT_TRUE(is_done);

  scheduling_group->Stop();
  timer_worker.Stop();
  timer_worker.Join();
  for (auto&& t : workers) {
    t.join();
  }
}

TEST(SchedulingGroup, FiberYieldOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestFiberYield(name);
    std::cout << "name:" << name << std::endl;
  }
}

}  // namespace trpc::fiber::detail
