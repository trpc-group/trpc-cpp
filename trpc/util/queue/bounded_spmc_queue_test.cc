//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/util/queue/bounded_spmc_queue.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "gtest/gtest.h"

namespace trpc::testing {

// T is pointer
TEST(BoundedSPMCQueueTest, SingleThreadTest) {
  trpc::BoundedSPMCQueue<int*> q(2);

  ASSERT_EQ(q.Capacity(), 2);
  ASSERT_EQ(q.Size(), 0);

  int* p1 = new int(1);
  ASSERT_TRUE(q.Enqueue(p1));
  ASSERT_EQ(q.Size(), 1);

  int* p2 = new int(2);
  ASSERT_TRUE(q.Enqueue(p2));
  ASSERT_EQ(q.Size(), 2);

  int* p3 = new int(3);
  ASSERT_FALSE(q.Enqueue(p3));
  ASSERT_EQ(q.Size(), 2);

  delete p3;
  p3 = nullptr;

  int* data = q.Dequeue();

  ASSERT_TRUE(data != nullptr);
  ASSERT_EQ(2, *data);
  ASSERT_EQ(q.Size(), 1);

  delete data;
  data = nullptr;

  data = q.Dequeue();
  ASSERT_TRUE(data != nullptr);
  ASSERT_EQ(1, *data);
  ASSERT_EQ(q.Size(), 0);

  delete data;
  data = nullptr;

  ASSERT_TRUE(q.Dequeue() == nullptr);
}

TEST(BoundedSPMCQueueTest, MultiThreadStealTest) {
  std::atomic<bool> exiting{false};
  std::mutex mutex;
  std::condition_variable cond;
  std::atomic<uint32_t> count{0};

  trpc::BoundedSPMCQueue<int*> spmc_queue(50000);

  std::vector<std::thread> consumer_threads;
  consumer_threads.resize(4);

  for (auto&& t : consumer_threads) {
    t = std::thread([&] {
      while (!exiting.load(std::memory_order_relaxed)) {
        int* p = spmc_queue.Steal();
        if (p) {
          ++count;
          delete p;
          p = nullptr;
        } else {
          std::unique_lock lk(mutex);
          cond.wait(lk, [&] { return exiting.load(std::memory_order_relaxed) || spmc_queue.Size() > 0; });
        }
      }
    });
  }


  std::thread produce_thread([&] {
    for (size_t i = 0; i < 40000; ++i) {
      int* p = new int(i);

      if (spmc_queue.Enqueue(p)) {
        std::scoped_lock _(mutex);
        cond.notify_one();
      }
    }
  });

  produce_thread.join();

  while (count.load(std::memory_order_relaxed) != 4 * 10000) {
    usleep(1000);
  }

  {
    std::scoped_lock _(mutex);
    exiting.store(true, std::memory_order_relaxed);
    cond.notify_all();
  }

  for (auto&& t : consumer_threads) {
    t.join();
  }

  ASSERT_EQ(count.load(std::memory_order_relaxed), 4 * 10000);
}

}  // namespace trpc::testing
