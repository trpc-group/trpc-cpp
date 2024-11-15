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

#include "trpc/coroutine/fiber.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "gflags/gflags.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

#define COUT std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|"

namespace trpc {

TEST(Fiber, StartWithDispatch) {
  RunAsFiber([] {
    for (int k = 0; k != 2; ++k) {
      constexpr auto N = 2;

      std::atomic<std::size_t> run{};
      std::vector<Fiber> fs(N);

      for (int i = 0; i != N; ++i) {
        auto we_re_in = std::this_thread::get_id();
        Fiber::Attributes attr;
        attr.launch_policy = fiber::Launch::Dispatch;
        fs[i] = Fiber(attr, [&, we_re_in] {
          ASSERT_EQ(we_re_in, std::this_thread::get_id());
          ++run;
        });
      }

      std::atomic<std::size_t> max_sleep{};
      while (run != N && max_sleep < 5000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        max_sleep++;
      }

      for (auto&& e : fs) {
        ASSERT_TRUE(e.Joinable());
        e.Join();
      }

      ASSERT_EQ(N, run);
    }
  });
}

TEST(Fiber, BatchStartUseLatch) {
  RunAsFiber([&] {
    static constexpr auto B = 2;
    std::atomic<std::size_t> started{};
    FiberLatch l(B);

    std::vector<Function<void()>> procs;
    for (int j = 0; j != B; ++j) {
      procs.push_back([&] {
        ++started;
        l.CountDown();
      });
    }

    BatchStartFiberDetached(std::move(procs));

    l.Wait();

    ASSERT_EQ(B, started.load());
  });
}

TEST(Fiber, FiberYield) {
  RunAsFiber([&] {
    FiberLatch l(1);

    StartFiberDetached([&l] {
      auto tid = std::this_thread::get_id();

      while (tid == std::this_thread::get_id()) {
        FiberYield();
      }

      l.CountDown();
    });

    l.Wait();
  });
}

TEST(Fiber, GetFiberCount) {
  RunAsFiber([&] {
    FiberMutex lock;
    FiberConditionVariable cv;
    static constexpr auto B = 1000;
    FiberLatch l(B);
    FiberLatch l2(B);

    auto fiber_count = trpc::GetFiberCount();
    bool notify = false;

    for (int i = 0; i != B; ++i) {
      StartFiberDetached([&l, &l2, &lock, &cv, &notify] {
        l.CountDown();
        {
          std::unique_lock lk(lock);
          while(!notify){
            cv.wait(lk);
          }
        }
        l2.CountDown();
      });
    }

    // BatchStartFiberDetached(std::move(procs));
    l.Wait();
    auto start_fiber_count = trpc::GetFiberCount() - fiber_count;

    ASSERT_EQ(B, start_fiber_count);
    
    {
      std::unique_lock lk(lock);
      cv.notify_all();
      notify = true;
    }

    l2.Wait();

    while ((trpc::GetFiberCount() - fiber_count) != 0) {
      usleep(100);
    }

    ASSERT_EQ(0, trpc::GetFiberCount() - fiber_count);
  });
}

TEST(Fiber, FiberSleepInFiberContext) {
  RunAsFiber([] {
    FiberLatch l(1);
    StartFiberDetached([&l] {
      // Test FiberSleepFor.
      auto sleep_for = 100 * std::chrono::milliseconds(1);
      auto start = ReadSystemClock();  // Used system_clock intentionally.
      FiberSleepFor(sleep_for);

      auto use_time = (ReadSystemClock() - start) / std::chrono::milliseconds(1);
      auto expect_time = sleep_for / std::chrono::milliseconds(1);
      auto error_time = use_time - expect_time;
      COUT << "use_time:" << use_time << ",expect_time:" << expect_time
           << ",error_time:" << error_time << std::endl;

      ASSERT_NEAR((ReadSystemClock() - start) / std::chrono::milliseconds(1),
                  sleep_for / std::chrono::milliseconds(1), 100);

      // Test FiberSleepUntil.
      auto sleep_until = ReadSystemClock() + 100 * std::chrono::milliseconds(1);
      FiberSleepUntil(sleep_until);

      use_time = (ReadSystemClock().time_since_epoch()) / std::chrono::milliseconds(1);
      expect_time = sleep_until.time_since_epoch() / std::chrono::milliseconds(1);
      error_time = use_time - expect_time;

      COUT << "use_time:" << use_time << ",expect_time:" << expect_time
           << ",error_time:" << error_time << std::endl;

      ASSERT_NEAR(use_time, expect_time, 20);

      l.CountDown();
    });

    l.Wait();
  });
}

TEST(Fiber, FiberSleepInPthreadContext) {
  auto sleep_for = 100 * std::chrono::milliseconds(1);
  auto start = ReadSystemClock();
  FiberSleepFor(sleep_for);

  auto use_time = (ReadSystemClock() - start) / std::chrono::milliseconds(1);
  auto expect_time = sleep_for / std::chrono::milliseconds(1);
  auto error_time = use_time - expect_time;
  COUT << "use_time:" << use_time << ",expect_time:" << expect_time
       << ",error_time:" << error_time << std::endl;

  ASSERT_NEAR((ReadSystemClock() - start) / std::chrono::milliseconds(1),
               sleep_for / std::chrono::milliseconds(1), 20);

  auto sleep_until = ReadSystemClock() + 100 * std::chrono::milliseconds(1);
  FiberSleepUntil(sleep_until);

  use_time = (ReadSystemClock().time_since_epoch()) / std::chrono::milliseconds(1);
  expect_time = sleep_until.time_since_epoch() / std::chrono::milliseconds(1);
  error_time = use_time - expect_time;

  COUT << "use_time:" << use_time << ",expect_time:" << expect_time
       << ",error_time:" << error_time << std::endl;

  ASSERT_NEAR(use_time, expect_time, 20);
}

TEST(Fiber, FiberMutexInFiberContext) {
  RunAsFiber([] {
    static constexpr auto B = 1000;
    FiberLatch l(B);

    FiberMutex m;
    int value = 0;
    for (int i = 0; i != B; ++i) {
      StartFiberDetached([&l, &m, &value] {
        std::scoped_lock _(m);
        ++value;

        l.CountDown();
      });
    }

    l.Wait();

    COUT << "value:" << value << std::endl;

    ASSERT_EQ(B, value);
  });
}

TEST(Fiber, FiberMutexInPthreadContext) {
  static constexpr auto B = 64;
  Latch l(B);

  FiberMutex m;
  ASSERT_EQ(true, m.try_lock());
  m.unlock();

  int value = 0;
  std::thread ts[B];
  for (int i = 0; i != B; ++i) {
    ts[i] = std::thread([&l, &m, &value] {
      std::scoped_lock _(m);
      ++value;

      l.count_down();
    });
  }

  l.wait();
  for (auto&& t : ts) {
    t.join();
  }

  COUT << "value:" << value << std::endl;
  ASSERT_EQ(B, value);
}

TEST(Fiber, FiberMutexInMixedContext) {
  RunAsFiber([] {
    static constexpr auto B = 64;
    FiberLatch l(2 * B);
    FiberMutex m;

    int value = 0;
    std::thread ts[B];
    for (int i = 0; i != B; ++i) {
      ts[i] = std::thread([&l, &m, &value] {
        std::scoped_lock _(m);
        ++value;
        l.CountDown();
      });

      StartFiberDetached([&l, &m, &value] {
        std::scoped_lock _(m);
        ++value;
        l.CountDown();
      });
    }

    l.Wait();
    for (auto&& t : ts) {
      t.join();
    }

    COUT << "value:" << value << std::endl;
    ASSERT_EQ(2 * B, value);
  });
}

// Passing `c` as non-const reference to test `std::ref` (see below.).
void Product(int a, int b, int& c) { c = a * b; }

TEST(Fiber, CallWithArgs) {
  RunAsFiber([]() {
    // Test lambda
    Fiber([](const char* hello) { ASSERT_EQ(hello, "hello"); }, "hello").Join();

    Fiber(
        [](auto&& First, auto&&... other) {
          auto ans = (First + ... + other);
          ASSERT_EQ(ans, 55);
        },
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
        .Join();

    // Test member method
    struct Add {
      void operator()(int a, int b, int c) const { ASSERT_EQ(a + b, c); }
    };

    const Add add;
    Fiber(std::ref(add), 2, 3, 5).Join();
    Fiber(&Add::operator(), &add, 1, 2, 3).Join();

    struct Worker {  // Noncopyable
      std::string s;
      void work(std::string_view s) { ASSERT_EQ("work...", s); }
      void operator()(const std::string& str) { s = str; }
      Worker() = default;
      Worker(Worker&&) = default;
      Worker(const Worker&) = delete;
      Worker& operator=(const Worker&) = delete;
    };

    Worker w;
    Fiber(&Worker::work, &w, "work...").Join();
    Fiber(&Worker::operator(), &w, "Work Test").Join();
    ASSERT_EQ(w.s, "Work Test");
    Fiber(std::move(w), "Move Test").Join();

    // Test template function
    std::vector vec{5, 4, 3, 2, 1};
    ASSERT_FALSE(std::is_sorted(vec.begin(), vec.end()));
    Fiber(&std::sort<std::vector<int>::iterator>, vec.begin(), vec.end())
        .Join();
    ASSERT_TRUE(std::is_sorted(vec.begin(), vec.end()));

    // Test function name
    int res = 0;
    Fiber(Product, 2, 5, std::ref(res)).Join();
    ASSERT_EQ(res, 10);

    // Test function address
    Fiber(&Product, 6, 7, std::ref(res)).Join();
    ASSERT_EQ(res, 42);

    // Test bind
    auto bind_function =
        std::bind(Product, 3, std::placeholders::_1, std::placeholders::_2);
    Fiber(bind_function, 5, std::ref(res)).Join();
    ASSERT_EQ(res, 15);

    // `std::pair` shouldn't be converted to `std::tuple` implicitly (by CTAD).
    Fiber([&](auto&& p) { res = p.first; }, std::make_pair(1, 2)).Join();
    EXPECT_EQ(1, res);
  });
}

}  // namespace trpc
