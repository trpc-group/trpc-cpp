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

#include "trpc/util/object_pool/shared_nothing.h"

#include <stdlib.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/object_pool/object_pool.h"

namespace trpc::object_pool {

struct C {
  int a;
  std::string s;
};

template <>
struct ObjectPoolTraits<C> {
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
};

TEST(SharedNothingTest, SharedNothingTest) {
  C* ptr = trpc::object_pool::New<C>();

  ASSERT_TRUE(ptr != nullptr);

  ptr->a = 1;
  ptr->s = "hello";

  trpc::object_pool::Delete<C>(ptr);

  ASSERT_TRUE(true);
}

TEST(SharedNothingTest, SharedNothingMultiNewDeleteTest) {
  std::vector<C*> items;

  for (size_t i = 0; i < 8192; ++i) {
    items.push_back(trpc::object_pool::New<C>());
  }

  for (size_t i = 0; i < 8192; ++i) {
    trpc::object_pool::Delete<C>(items[i]);
  }

  ASSERT_TRUE(true);
}

struct TestData {
  std::string s1;
  std::string s2;
};

template <>
struct ObjectPoolTraits<TestData> {
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
};

struct Item {
  TestData* p = nullptr;
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
    items_.emplace(std::move(job));
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
      trpc::object_pool::Delete<TestData>(item.p);
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
      item.p = trpc::object_pool::New<TestData>();
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

TEST(SharedNothingTest, SlotChunkManagerTest) {
  shared_nothing::detail::Slot<std::string> slot;
  slot.chunk_id = 1;
  shared_nothing::detail::SlotChunkManager<std::string> slot_chunk_manager;
  slot_chunk_manager.DoResize(100);
  shared_nothing::detail::SlotChunk<std::string>& slot_chunk = slot_chunk_manager.GetSlotChunk(slot.chunk_id);
  ASSERT_TRUE(slot_chunk_manager.Empty());
  slot_chunk_manager.PushFront(slot_chunk, slot.chunk_id);
  ASSERT_FALSE(slot_chunk_manager.Empty());
  slot.next = slot_chunk.freeslot.head;
  slot_chunk.freeslot.head = &slot;

  shared_nothing::detail::SlotChunk<std::string>& chunk = slot_chunk_manager.Front();
  ASSERT_TRUE(chunk.freeslot.head != nullptr);
  slot_chunk_manager.PopFront();
  ASSERT_TRUE(slot_chunk_manager.Empty());
}

struct D {
  int a;
  std::string s;
};

template <>
struct ObjectPoolTraits<D> {
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
  static constexpr size_t kMaxObjectNum = 1024;
};

template <class T>
static void create_over_limit_objects(std::vector<T*>& vec, size_t max_num) {
  using shared_nothing::kDefaultMaxObjectNum;
  using shared_nothing::detail::kMinChunkSize;
  using shared_nothing::detail::kPageSize;
  size_t c_size = sizeof(shared_nothing::detail::Slot<T>);

  auto chunk_size = std::max<uint32_t>(kMinChunkSize, kPageSize / static_cast<uint32_t>(c_size));
  size_t max_cnt = max_num + chunk_size;

  vec.resize(max_cnt);
  for (size_t i = 0; i < max_cnt; i++) {
    T* ptr = trpc::object_pool::New<T>();
    vec.push_back(ptr);
  }
}

TEST(SharedNothingTest, SharedNothingOverLimit) {
  using shared_nothing::kDefaultMaxObjectNum;
  using shared_nothing::detail::kMinChunkSize;
  using shared_nothing::detail::kPageSize;
  {
    // Test with more objects than the default number.
    std::vector<C*> vec;
    create_over_limit_objects<C>(vec, kDefaultMaxObjectNum);
    C* ptr = trpc::object_pool::New<C>();
    auto* slot = reinterpret_cast<shared_nothing::detail::Slot<C>*>(ptr);
    ASSERT_TRUE(slot->need_free_to_system);
    ASSERT_TRUE(shared_nothing::GetTlsStatistics<C>().slot_from_system > 0);
    vec.push_back(ptr);
    for (auto& t : vec) {
      trpc::object_pool::Delete<C>(t);
    }
    ASSERT_TRUE(shared_nothing::GetTlsStatistics<C>().slot_from_system == 0);
  }
  {
    // Test with a specified number of objects.
    std::vector<D*> vec;
    create_over_limit_objects<D>(vec, ObjectPoolTraits<D>::kMaxObjectNum);
    D* ptr = trpc::object_pool::New<D>();
    auto* slot = reinterpret_cast<shared_nothing::detail::Slot<D>*>(ptr);
    ASSERT_TRUE(slot->need_free_to_system);
    ASSERT_TRUE(shared_nothing::GetTlsStatistics<D>().slot_from_system > 0);
    vec.push_back(ptr);
    for (auto& t : vec) {
      trpc::object_pool::Delete<D>(t);
    }
    ASSERT_TRUE(shared_nothing::GetTlsStatistics<D>().slot_from_system == 0);
  }
  {
    std::vector<D*> vec;
    // Release across threads.
    std::thread thread1([&vec]() {
      create_over_limit_objects<D>(vec, ObjectPoolTraits<D>::kMaxObjectNum);
      D* ptr = trpc::object_pool::New<D>();
      auto* slot = reinterpret_cast<shared_nothing::detail::Slot<D>*>(ptr);
      ASSERT_TRUE(slot->need_free_to_system);
      ASSERT_TRUE(shared_nothing::GetTlsStatistics<D>().slot_from_system > 0);
      vec.push_back(ptr);
    });

    thread1.join();
    std::thread thread2([&vec]() {
      for (auto& t : vec) {
        trpc::object_pool::Delete<D>(t);
      }
    });
    thread2.join();
  }
}

}  // namespace trpc::object_pool
