//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/waitable_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"

#include <atomic>
#include <deque>
#include <numeric>
#include <queue>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_worker.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/fiber/detail/testing.h"
#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/chrono/tsc.h"
#include "trpc/util/random.h"

using namespace std::literals;

namespace trpc::fiber::detail {

namespace {

void Sleep(std::chrono::nanoseconds ns) {
  WaitableTimer wt(ReadSteadyClock() + ns);
  wt.wait();
}

void RandomDelay() {
  auto round = Random(100);
  for (int i = 0; i != round; ++i) {
    ReadTsc();
  }
}

}  // namespace

constexpr std::string_view kSchedulingNames[] = {kSchedulingV1, kSchedulingV2};

// **Concurrently** call `cb` for `times`.
template <class F>
void RunInFiber(std::string_view scheduling_name, std::size_t times, F cb) {
  std::atomic<std::size_t> called{};
  auto sg = std::make_unique<SchedulingGroup>(std::vector<unsigned>{}, 16, scheduling_name);
  TimerWorker timer_worker(sg.get());
  sg->SetTimerWorker(&timer_worker);
  std::deque<FiberWorker> workers;

  for (int i = 0; i != 16; ++i) {
    workers.emplace_back(sg.get(), i).Start();
  }
  timer_worker.Start();
  for (size_t i = 0; i != times; ++i) {
    testing::StartFiberEntityInGroup(sg.get(), [&, i] {
      cb(i);
      ++called;
    });
  }
  while (called != times) {
    std::this_thread::sleep_for(100ms);
  }
  sg->Stop();
  timer_worker.Stop();
  for (auto&& w : workers) {
    w.Join();
  }
  timer_worker.Join();
}

TEST(Waitable, WaitableTimerOnScheduling) {
  for (auto& name : kSchedulingNames) {
    RunInFiber(name, 100, [](auto) {
      auto now = ReadSteadyClock();
      WaitableTimer wt(now + 1s);
      wt.wait();
      ASSERT_TRUE(true);
    });
  }
}

TEST(Waitable, MutexOnScheduling) {
  for (auto& name : kSchedulingNames) {
    for (int i = 0; i != 10; ++i) {
      Mutex m;
      int value = 0;
      RunInFiber(name, 10000, [&](auto) {
        std::scoped_lock _(m);
        ++value;
      });
      ASSERT_EQ(10000, value);
    }
  }
}

TEST(Waitable, MutexInPthreadContext) {
  static constexpr auto B = 64;

  Mutex m;
  ASSERT_EQ(true, m.try_lock());
  m.unlock();

  int value = 0;
  std::thread ts[B];
  for (int i = 0; i != B; ++i) {
    ts[i] = std::thread([&m, &value] {
      std::scoped_lock _(m);
      ++value;
    });
  }

  for (auto&& t : ts) {
    t.join();
  }

  ASSERT_EQ(B, value);
}

TEST(Waitable, MutexInMixedContext) {
  RunAsFiber([] {
    static constexpr auto B = 64;
    Mutex m;

    int value = 0;
    std::thread ts[B];
    std::vector<trpc::Fiber> fbs(B);
    for (int i = 0; i != B; ++i) {
      ts[i] = std::thread([&m, &value] {
        std::scoped_lock _(m);
        ++value;
      });

      fbs[i] = trpc::Fiber([&m, &value] {
        std::scoped_lock _(m);
        ++value;
      });
    }

    for (auto&& t : ts) {
      t.join();
    }

    for (auto&& e : fbs) {
      e.Join();
    }

    ASSERT_EQ(2 * B, value);
  });
}
TEST(Waitable, ConditionVariableInPthreadContext) {
  constexpr auto N = 64;
  std::atomic<std::size_t> run{0};
  Mutex lock[N];
  ConditionVariable cv[N];
  bool set[N] = {false};
  std::vector<std::thread> prod(N);
  std::vector<std::thread> cons(N);

  for (int i = 0; i != N; ++i) {
    prod[i] = std::thread([&run, i, &cv, &lock, &set] {
      trpc::FiberSleepFor(trpc::Random(20) * std::chrono::milliseconds(1));
      std::unique_lock lk(lock[i]);
      cv[i].wait(lk, [&] { return set[i]; });
      ++run;
    });

    cons[i] = std::thread([&run, i, &cv, &lock, &set] {
      trpc::FiberSleepFor(trpc::Random(20) * std::chrono::milliseconds(1));
      std::scoped_lock _(lock[i]);
      set[i] = true;
      cv[i].notify_one();
      ++run;
    });
  }

  for (auto&& e : prod) {
    ASSERT_TRUE(e.joinable());
    e.join();
  }

  for (auto&& e : cons) {
    ASSERT_TRUE(e.joinable());
    e.join();
  }

  ASSERT_EQ(N * 2, run);
}

TEST(Waitable, ConditionVariableNotifyPthreadFromFiber) {
  RunAsFiber([] {
    constexpr auto K = 64;
    std::atomic<std::size_t> executed{0};
    Mutex lock[K];
    ConditionVariable cv[K];
    bool set[K] = {false};
    std::vector<std::thread> prod(K);
    std::vector<trpc::Fiber> cons(K);

    for (int i = 0; i != K; ++i) {
      prod[i] = std::thread([&executed, i, &cv, &lock, &set] {
        trpc::FiberSleepFor(trpc::Random(20) * std::chrono::milliseconds(1));
        std::unique_lock lk(lock[i]);
        cv[i].wait(lk, [&] { return set[i]; });
        ++executed;
      });

      cons[i] = trpc::Fiber([&executed, i, &cv, &lock, &set] {
        trpc::FiberSleepFor(trpc::Random(20) * std::chrono::milliseconds(1));
        std::scoped_lock _(lock[i]);
        set[i] = true;
        cv[i].notify_one();
        ++executed;
      });
    }

    for (auto&& e : prod) {
      ASSERT_TRUE(e.joinable());
      e.join();
    }

    for (auto&& e : cons) {
      ASSERT_TRUE(e.Joinable());
      e.Join();
    }

    ASSERT_EQ(K * 2, executed);
  });
}

TEST(Waitable, ConditionVariableNotifyFiberFromPthread) {
  RunAsFiber([] {
    constexpr auto N = 64;
    std::atomic<std::size_t> run{0};
    Mutex lock[N];
    ConditionVariable cv[N];
    bool set[N] = {false};
    std::vector<trpc::Fiber> prod(N);
    std::vector<std::thread> cons(N);

    for (int i = 0; i != N; ++i) {
      prod[i] = trpc::Fiber([&run, i, &cv, &lock, &set] {
        trpc::FiberSleepFor(trpc::Random(20) * std::chrono::milliseconds(1));
        std::scoped_lock _(lock[i]);
        set[i] = true;
        cv[i].notify_one();
        ++run;
      });

      cons[i] = std::thread([&run, i, &cv, &lock, &set] {
        trpc::FiberSleepFor(trpc::Random(20) * std::chrono::milliseconds(1));
        std::unique_lock lk(lock[i]);
        cv[i].wait(lk, [&] { return set[i]; });
        ++run;
      });
    }

    for (auto&& e : prod) {
      ASSERT_TRUE(e.Joinable());
      e.Join();
    }

    for (auto&& e : cons) {
      ASSERT_TRUE(e.joinable());
      e.join();
    }

    ASSERT_EQ(N * 2, run);
  });
}

void TestConditionVariable(std::string_view scheduling_name) {
  constexpr auto N = 10000;

  for (int i = 0; i != 10; ++i) {
    Mutex m[N];
    ConditionVariable cv[N];
    std::queue<int> queue[N];
    std::atomic<std::size_t> read{}, write{};
    int sum[N] = {};

    // We, in fact, are passing data between two scheduling groups.
    //
    // This should work anyway.
    auto prods = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        auto to = Random(N - 1);
        std::scoped_lock _(m[to]);
        queue[to].push(index);
        cv[to].notify_one();
        ++write;
      });
    });
    auto signalers = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        std::unique_lock lk(m[index]);
        bool exit = false;
        while (!exit) {
          auto&& mq = queue[index];
          cv[index].wait(lk, [&] { return !mq.empty(); });
          ASSERT_TRUE(lk.owns_lock());
          while (!mq.empty()) {
            if (mq.front() == -1) {
              exit = true;
              break;
            }
            sum[index] += mq.front();
            ++read;
            mq.pop();
          }
        }
      });
    });
    prods.join();
    RunInFiber(scheduling_name, N, [&](auto index) {
      std::scoped_lock _(m[index]);
      queue[index].push(-1);
      cv[index].notify_one();
    });
    signalers.join();
    ASSERT_EQ((N - 1) * N / 2, std::accumulate(std::begin(sum), std::end(sum), 0));
  }
}

TEST(Waitable, ConditionVariableOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestConditionVariable(name);
  }
}

void TestConditionVariable2(std::string_view scheduling_name) {
  constexpr auto N = 1000;

  for (int i = 0; i != 100; ++i) {
    Mutex m[N];
    ConditionVariable cv[N];
    bool f[N] = {};
    std::atomic<int> sum{};
    auto prods = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        std::scoped_lock _(m[index]);
        f[index] = true;
        cv[index].notify_one();
      });
    });
    auto signalers = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        std::unique_lock lk(m[index]);
        cv[index].wait(lk, [&] { return f[index]; });
        ASSERT_TRUE(lk.owns_lock());
        sum += index;
      });
    });
    prods.join();
    signalers.join();
    ASSERT_EQ((N - 1) * N / 2, sum);
  }
}

TEST(Waitable, ConditionVariable2OnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestConditionVariable2(name);
  }
}

void TestConditionVariableNoTimeout(std::string_view scheduling_name) {
  constexpr auto N = 1000;
  std::atomic<std::size_t> done{};
  Mutex m;
  ConditionVariable cv;
  std::thread([&] {
    RunInFiber(scheduling_name, N, [&](auto index) {
      std::unique_lock lk(m);
      done += cv.wait_until(lk, ReadSteadyClock() + 100s);
    });
  }).detach();
  std::thread([&] {
    RunInFiber(scheduling_name, 1, [&](auto index) {
      while (done != N) {
        cv.notify_all();
      }
    });
  }).join();
  ASSERT_EQ(N, done);
}

TEST(Waitable, ConditionVariableNoTimeoutOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestConditionVariableNoTimeout(name);
  }
}

void TestConditionVariableTimeout(std::string_view scheduling_name) {
  constexpr auto N = 1000;
  std::atomic<std::size_t> timed_out{};
  Mutex m;
  ConditionVariable cv;
  RunInFiber(scheduling_name, N, [&](auto index) {
    std::unique_lock lk(m);
    timed_out += !cv.wait_until(lk, ReadSteadyClock() + 1ms);
  });
  ASSERT_EQ(N, timed_out);
}

TEST(Waitable, ConditionVariableTimeoutOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestConditionVariableTimeout(name);
  }
}

void TestConditionVariableRace(std::string_view scheduling_name) {
  constexpr auto N = 1000;

  for (int i = 0; i != 10; ++i) {
    Mutex m;
    ConditionVariable cv;
    std::atomic<int> sum{};
    auto prods = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        for (int j = 0; j != 100; ++j) {
          Sleep(Random(100) * 1us);
          std::scoped_lock _(m);
          cv.notify_all();
        }
      });
    });
    auto signalers = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        for (int j = 0; j != 100; ++j) {
          std::unique_lock lk(m);
          cv.wait_until(lk, ReadSteadyClock() + 50us);
          ASSERT_TRUE(lk.owns_lock());
        }
        sum += index;
      });
    });
    prods.join();
    signalers.join();
    ASSERT_EQ((N - 1) * N / 2, sum);
  }
}

TEST(Waitable, ConditionVariableRaceOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestConditionVariableRace(name);
  }
}

void TestExitBarrier(std::string_view scheduling_name) {
  constexpr auto N = 10000;

  for (int i = 0; i != 10; ++i) {
    std::deque<ExitBarrier> l;
    std::atomic<std::size_t> waited{};

    for (int j = 0; j != N; ++j) {
      l.emplace_back();
    }

    auto counters = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        l[index].UnsafeCountDown(l[index].GrabLock());
      });
    });
    auto waiters = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        l[index].Wait();
        ++waited;
      });
    });
    counters.join();
    waiters.join();
    ASSERT_EQ(N, waited);
  }
}

TEST(Waitable, ExitBarrierOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestExitBarrier(name);
  }
}

void TestEvent(std::string_view scheduling_name) {
  constexpr auto N = 10000;

  for (int i = 0; i != 10; ++i) {
    std::deque<Event> evs;
    std::atomic<std::size_t> waited{};

    for (int j = 0; j != N; ++j) {
      evs.emplace_back();
    }

    auto counters = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        RandomDelay();
        evs[index].Set();
      });
    });
    auto waiters = std::thread([&] {
      RunInFiber(scheduling_name, N, [&](auto index) {
        RandomDelay();
        evs[index].Wait();
        ++waited;
      });
    });
    counters.join();
    waiters.join();
    ASSERT_EQ(N, waited);
  }
}

TEST(Waitable, EventWaitInPthreadSetFromFiber) {
  RunAsFiber([]() {
    for (int i = 0; i != 100; ++i) {
      auto ev = std::make_unique<Event>();
      std::thread t([&] {
        trpc::FiberSleepFor(trpc::Random(10) * std::chrono::milliseconds(1));
        ev->Wait();
      });
      trpc::FiberSleepFor(trpc::Random(10) * std::chrono::milliseconds(1));
      ev->Set();
      t.join();
    }
  });
}

TEST(Waitable, EventWaitInPthreadSetFromPthread) {
  for (int i = 0; i != 100; ++i) {
    auto ev = std::make_unique<Event>();
    std::thread t([&] {
      trpc::FiberSleepFor(trpc::Random(10) * std::chrono::milliseconds(1));
      ev->Wait();
    });
    trpc::FiberSleepFor(trpc::Random(10) * std::chrono::milliseconds(1));
    ev->Set();
    t.join();
  }
}

TEST(Waitable, EventOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestEvent(name);
  }
}

TEST(Waitable, EventFreeOnWakeupOnScheduling) {
  // This UT detects a use-after-free race, but it's can only be revealed by
  // UBSan in most cases, unfortunately.
  for (auto& name : kSchedulingNames) {
    RunInFiber(name, 100, [&](auto index) {
      for (int i = 0; i != 100; ++i) {
        auto ev = std::make_unique<Event>();
        std::thread([&] { ev->Set(); }).detach();
        ev->Wait();
        ev = nullptr;
        usleep(10000);
      }
    });
  }
}

void TestOneshotTimedEventTest(std::string_view scheduling_name) {
  RunInFiber(scheduling_name, 1, [&](auto index) {
    OneshotTimedEvent ev1(ReadSteadyClock() + 1s), ev2(ReadSteadyClock() + 10ms);

    auto start = ReadSteadyClock();
    ev2.Wait();
    EXPECT_LT((ReadSteadyClock() - start) / 1ms, 100);

    auto t = std::thread([&] {
      std::this_thread::sleep_for(500ms);
      ev1.Set();
    });
    start = ReadSteadyClock();
    ev1.Wait();
    EXPECT_NEAR((ReadSteadyClock() - start) / 1ms, 500, 100);
    t.join();
  });
}

TEST(OneshotTimedEvent, OneshotTimedEventTestOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestOneshotTimedEventTest(name);
  }
}

void TestOneshotTimedEventTorture(std::string_view scheduling_name) {
  constexpr auto N = 10000;

  RunInFiber(scheduling_name, 1, [&](auto) {
    for (int i = 0; i != 10; ++i) {
      std::deque<OneshotTimedEvent> evs;
      std::atomic<std::size_t> waited{};

      for (int j = 0; j != N; ++j) {
        evs.emplace_back(ReadSteadyClock() + Random(1000) * 1ms);
      }
      auto counters = std::thread([&] {
        RunInFiber(scheduling_name, N, [&](auto index) {
          RandomDelay();
          evs[index].Set();
        });
      });
      auto waiters = std::thread([&] {
        RunInFiber(scheduling_name, N, [&](auto index) {
          RandomDelay();
          evs[index].Wait();
          ++waited;
        });
      });
      counters.join();
      waiters.join();
      ASSERT_EQ(N, waited);
    }
  });
}

TEST(OneshotTimedEvent, OneshotTimedEventTortureOnScheduling) {
  for (auto& name : kSchedulingNames) {
    TestOneshotTimedEventTorture(name);
  }
}

}  // namespace trpc::fiber::detail
