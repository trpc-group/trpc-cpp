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

#include "trpc/util/buffer/memory_pool/shared_nothing_memory_pool.h"

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::memory_pool::shared_nothing {

namespace testing {

TEST(SharedNothingTest, SingleNewDeleteTest) {
  auto* block = Allocate();

  ASSERT_TRUE(block != nullptr);
  ASSERT_FALSE(block->need_free_to_system);
  ASSERT_TRUE(block->cpu_id < detail::kInvalidCpuId);

  Deallocate(block);
}

TEST(SharedNothingTest, MultiNewDeleteTest) {
  std::vector<detail::Block*> items;

  for (size_t i = 0; i < 8192 * 64 + 1; ++i) {
    items.push_back(Allocate());
  }
  auto* last_block = items[items.size() - 1];
  ASSERT_TRUE(last_block->need_free_to_system);

  for (size_t i = 0; i < 8192 * 64 + 1; ++i) {
    Deallocate(items[i]);
  }
}

TEST(SharedNothingTest, SharedNothingMemPoolImpTest) {
  detail::SharedNothingMemPoolImp* pool = new detail::SharedNothingMemPoolImp();
  ASSERT_TRUE(pool);
  for (int i = 0; i < 16 * 1024 + 1; i++) {
    pool->Allocate();
  }
  delete pool;
}

struct Item {
  detail::Block* p = nullptr;
  std::atomic<size_t>* execute_num = nullptr;
};

class ThreadPool {
 public:
  struct Options {
    std::size_t thread_num = 1;
  };

  explicit ThreadPool(Options&& options) : options_(std::move(options)) {}

  bool Start() {
    assert(options_.thread_num > 0);
    threads_.resize(options_.thread_num);

    for (auto&& t : threads_) {
      t = std::thread([this] { this->Run(); });
    }

    return true;
  }

  void Push(Item&& job) {
    std::scoped_lock _(lock_);
    items_.emplace(job);
    cond_.notify_one();
  }

  void Stop() {
    std::scoped_lock _(lock_);
    exiting_.store(true, std::memory_order_relaxed);
    cond_.notify_all();
  }

  void Join() {
    for (auto&& t : threads_) {
      t.join();
    }
  }

 private:
  void Run() {
    while (true) {
      std::unique_lock lk(lock_);
      cond_.wait(lk, [&] { return exiting_.load(std::memory_order_relaxed) || !items_.empty(); });
      if (exiting_.load(std::memory_order_relaxed)) {
        break;
      }
      auto item = std::move(items_.front());
      items_.pop();
      lk.unlock();
      Deallocate(item.p);
      ++(*(item.execute_num));
    }
  }

 private:
  Options options_;

  std::atomic<bool> exiting_{false};
  std::vector<std::thread> threads_;

  std::mutex lock_;
  std::condition_variable cond_;
  std::queue<Item> items_;
};

TEST(SharedNothingTest, SharedNothingMultiThreadTest) {
  ThreadPool::Options options;
  options.thread_num = 2;

  ThreadPool tp(std::move(options));

  tp.Start();

  sleep(2);

  std::function<void()> fun = [&tp] {
    std::atomic<size_t> task_execute_num = 0;

    size_t execute_num = 1000;
    for (size_t i = 0; i < execute_num; ++i) {
      Item item;
      item.p = Allocate();
      item.execute_num = &task_execute_num;
      tp.Push(std::move(item));
    }

    while (task_execute_num != execute_num) {
      usleep(10);
    }

    ASSERT_TRUE(task_execute_num == execute_num);
  };

  std::vector<std::thread> threads;
  threads.resize(4);

  for (auto&& t : threads) {
    t = std::thread(fun);
  }

  for (auto&& t : threads) {
    t.join();
  }

  tp.Stop();
  tp.Join();

  ASSERT_TRUE(true);
}
}  // namespace testing
}  // namespace trpc::memory_pool::shared_nothing
