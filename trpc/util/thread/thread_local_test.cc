//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/thread_local_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/thread_local.h"

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <atomic>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/latch.h"

namespace trpc::testing {

struct Widget {
  static int totalVal_;
  int val_;
  ~Widget() { totalVal_ += val_; }
};

int Widget::totalVal_ = 0;

TEST(ThreadLocal, BasicDestructor) {
  Widget::totalVal_ = 0;
  ThreadLocal<Widget> w;
  std::thread([&w]() { w->val_ += 10; }).join();
  EXPECT_EQ(10, Widget::totalVal_);
}

TEST(ThreadLocal, SimpleRepeatDestructor) {
  Widget::totalVal_ = 0;
  {
    ThreadLocal<Widget> w;
    w->val_ += 10;
  }
  {
    ThreadLocal<Widget> w;
    w->val_ += 10;
  }
  EXPECT_EQ(20, Widget::totalVal_);
}

TEST(ThreadLocal, InterleavedDestructors) {
  Widget::totalVal_ = 0;
  std::unique_ptr<ThreadLocal<Widget>> w;
  int wVersion = 0;
  const int wVersionMax = 2;
  int thIter = 0;
  std::mutex lock;
  auto th = std::thread([&]() {
    int wVersionPrev = 0;
    while (true) {
      while (true) {
        std::lock_guard<std::mutex> g(lock);
        if (wVersion > wVersionMax) {
          return;
        }
        if (wVersion > wVersionPrev) {
          // We have a new version of w, so it should be initialized to zero
          EXPECT_EQ(0, (*w)->val_);
          break;
        }
      }
      std::lock_guard<std::mutex> g(lock);
      wVersionPrev = wVersion;
      (*w)->val_ += 10;
      ++thIter;
    }
  });
  for (size_t i = 0; i < wVersionMax; ++i) {
    int thIterPrev = 0;
    {
      std::lock_guard<std::mutex> g(lock);
      thIterPrev = thIter;
      w = std::make_unique<ThreadLocal<Widget>>();
      ++wVersion;
    }
    while (true) {
      std::lock_guard<std::mutex> g(lock);
      if (thIter > thIterPrev) {
        break;
      }
    }
  }
  {
    std::lock_guard<std::mutex> g(lock);
    wVersion = wVersionMax + 1;
  }
  th.join();
  EXPECT_EQ(wVersionMax * 10, Widget::totalVal_);
}

class SimpleThreadCachedInt {
  ThreadLocal<int> val_;

 public:
  void add(int val) { *val_ += val; }

  int read() {
    int ret = 0;
    val_.ForEach([&](const int* p) { ret += *p; });
    return ret;
  }
};

TEST(ThreadLocal, AccessAllThreadsCounter) {
  const int kNumThreads = 256;
  SimpleThreadCachedInt stci[kNumThreads + 1];
  std::atomic<bool> run(true);
  std::atomic<int> totalAtomic{0};
  std::vector<std::thread> threads;
  // thread i will increment all the thread locals
  // in the range 0..i
  for (int i = 0; i < kNumThreads; ++i) {
    threads.push_back(std::thread([i,  // i needs to be captured by value
                                   &stci, &run, &totalAtomic]() {
      for (int j = 0; j <= i; ++j) {
        stci[j].add(1);
      }

      totalAtomic.fetch_add(1);
      while (run.load()) {
        usleep(100);
      }
    }));
  }
  while (totalAtomic.load() != kNumThreads) {
    usleep(100);
  }
  for (int i = 0; i <= kNumThreads; ++i) {
    EXPECT_EQ(kNumThreads - i, stci[i].read());
  }
  run.store(false);
  for (auto& t : threads) {
    t.join();
  }
}

TEST(ThreadLocal, resetNull) {
  ThreadLocal<int> tl;
  tl.Reset(new int(4));
  EXPECT_EQ(4, *tl.Get());
  tl.Reset();
  EXPECT_EQ(0, *tl.Get());
  tl.Reset(new int(5));
  EXPECT_EQ(5, *tl.Get());
}

struct Foo {
  ThreadLocal<int> tl;
};

TEST(ThreadLocal, Movable1) {
  Foo a;
  Foo b;
  EXPECT_TRUE(a.tl.Get() != b.tl.Get());
}

TEST(ThreadLocal, Movable2) {
  std::map<int, Foo> map;

  map[42];
  map[10];
  map[23];
  map[100];

  std::set<void*> tls;
  for (auto& m : map) {
    tls.insert(m.second.tl.Get());
  }

  // Make sure that we have 4 different instances of *tl
  EXPECT_EQ(4, tls.size());
}

using TLPInt = ThreadLocal<std::atomic<int>>;

template <typename Op, typename Check>
void StressAccessTest(Op op, Check check, size_t num_threads, size_t num_loops) {
  size_t kNumThreads = num_threads;
  size_t kNumLoops = num_loops;

  TLPInt ptr;
  ptr.Reset(new std::atomic<int>(0));
  std::atomic<bool> running{true};

  Latch l(kNumThreads + 1);

  std::vector<std::thread> threads;

  for (size_t k = 0; k < kNumThreads; ++k) {
    threads.emplace_back([&] {
      ptr.Reset(new std::atomic<int>(1));

      l.count_down();
      l.wait();

      while (running.load()) {
        op(ptr);
      }
    });
  }

  // wait for the threads to be up and running
  l.count_down();
  l.wait();

  for (size_t n = 0; n < kNumLoops; ++n) {
    int sum = 0;
    ptr.ForEach([&](const std::atomic<int>* p) { sum += *p; });
    check(sum, kNumThreads);
  }

  running.store(false);
  for (auto& t : threads) {
    t.join();
  }
}

TEST(ThreadLocal, StressAccessReset) {
  StressAccessTest([](TLPInt& ptr) { ptr.Reset(new std::atomic<int>(1)); },
                   [](size_t sum, size_t numThreads) { EXPECT_EQ(sum, numThreads); }, 16, 10);
}

TEST(ThreadLocal, StressAccessSet) {
  StressAccessTest([](TLPInt& ptr) { *ptr = 1; }, [](size_t sum, size_t numThreads) { EXPECT_EQ(sum, numThreads); }, 16,
                   100);
}

TEST(ThreadLocal, StressAccessRelease) {
  StressAccessTest(
      [](TLPInt& ptr) {
        auto* p = ptr.Leak();
        delete p;
        ptr.Reset(new std::atomic<int>(1));
      },
      [](size_t sum, size_t numThreads) { EXPECT_LE(sum, numThreads); }, 8, 4);
}

}  // namespace trpc::testing
