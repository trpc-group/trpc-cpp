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

#include "trpc/util/queue/bounded_mpmc_queue.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "gtest/gtest.h"

namespace trpc::testing {

// T is not pointer
TEST(BoundedMPMCQueueTest, SingleThreadTest1) {
  trpc::BoundedMPMCQueue<int> q;

  ASSERT_TRUE(q.Init(2));
  ASSERT_EQ(q.Capacity(), 2);
  ASSERT_EQ(q.Size(), 0);

  ASSERT_TRUE(q.Push(1));
  ASSERT_EQ(q.Size(), 1);
  ASSERT_TRUE(q.Push(2));
  ASSERT_EQ(q.Size(), 2);
  ASSERT_FALSE(q.Push(3));
  ASSERT_EQ(q.Size(), 2);

  int data = 0;
  ASSERT_TRUE(q.Pop(data));
  ASSERT_EQ(1, data);
  ASSERT_EQ(q.Size(), 1);

  ASSERT_TRUE(q.Pop(data));
  ASSERT_EQ(2, data);
  ASSERT_EQ(q.Size(), 0);

  ASSERT_FALSE(q.Pop(data));
}

TEST(BoundedMPMCQueueTest, MultiThreadTest1) {
  std::atomic<bool> exiting{false};
  std::mutex mutex;
  std::condition_variable cond;

  trpc::BoundedMPMCQueue<std::function<void()>> task_queue;
  ASSERT_TRUE(task_queue.Init(50000));

  std::vector<std::thread> consumer_threads;
  consumer_threads.resize(4);

  for (auto&& t : consumer_threads) {
    t = std::thread([&] {
      while (!exiting.load(std::memory_order_relaxed)) {
        std::function<void()> task;
        if (task_queue.Pop(task)) {
          task();
        } else {
          std::unique_lock lk(mutex);
          cond.wait(lk, [&] { return exiting.load(std::memory_order_relaxed) || task_queue.Size() > 0; });
        }
      }
    });
  }

  std::atomic<uint32_t> count{0};

  std::vector<std::thread> produce_threads;
  produce_threads.resize(4);

  for (auto&& t : produce_threads) {
    t = std::thread([&] {
      for (size_t i = 0; i < 10000; ++i) {
        auto task = [&count] {
          ++count;
        };

        if (task_queue.Push(std::move(task))) {
          std::scoped_lock _(mutex);
          cond.notify_one();
        }
      }
    });
  }

  for (auto&& t : produce_threads) {
    t.join();
  }

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

// T is pointer
TEST(BoundedMPMCQueueTest, SingleThreadTest2) {
  trpc::BoundedMPMCQueue<int*> q;

  ASSERT_TRUE(q.Init(2));
  ASSERT_EQ(q.Capacity(), 2);
  ASSERT_EQ(q.Size(), 0);

  int* p1 = new int(1);
  ASSERT_TRUE(q.Push(p1));
  ASSERT_EQ(q.Size(), 1);

  int* p2 = new int(2);
  ASSERT_TRUE(q.Push(p2));
  ASSERT_EQ(q.Size(), 2);

  int* p3 = new int(3);
  ASSERT_FALSE(q.Push(p3));
  ASSERT_EQ(q.Size(), 2);

  delete p3;
  p3 = nullptr;

  int* data{nullptr};

  ASSERT_TRUE(q.Pop(data));
  ASSERT_EQ(1, *data);
  ASSERT_EQ(q.Size(), 1);

  delete data;
  data = nullptr;

  ASSERT_TRUE(q.Pop(data));
  ASSERT_EQ(2, *data);
  ASSERT_EQ(q.Size(), 0);

  delete data;
  data = nullptr;

  ASSERT_FALSE(q.Pop(data));
}

TEST(BoundedMPMCQueueTest, MultiThreadTest2) {
  std::atomic<bool> exiting{false};
  std::mutex mutex;
  std::condition_variable cond;
  std::atomic<uint32_t> count{0};

  trpc::BoundedMPMCQueue<int*> mpmc_queue;
  ASSERT_TRUE(mpmc_queue.Init(50000));

  std::vector<std::thread> consumer_threads;
  consumer_threads.resize(4);

  for (auto&& t : consumer_threads) {
    t = std::thread([&] {
      while (!exiting.load(std::memory_order_relaxed)) {
        int* p{nullptr};
        if (mpmc_queue.Pop(p)) {
          ++count;
          delete p;
          p = nullptr;
        } else {
          std::unique_lock lk(mutex);
          cond.wait(lk, [&] { return exiting.load(std::memory_order_relaxed) || mpmc_queue.Size() > 0; });
        }
      }
    });
  }

  std::vector<std::thread> produce_threads;
  produce_threads.resize(4);

  for (auto&& t : produce_threads) {
    t = std::thread([&] {
      for (size_t i = 0; i < 10000; ++i) {
        int* p = new int(i);

        if (mpmc_queue.Push(p)) {
          std::scoped_lock _(mutex);
          cond.notify_one();
        }
      }
    });
  }

  for (auto&& t : produce_threads) {
    t.join();
  }

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
