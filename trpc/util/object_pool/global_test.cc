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

#include "trpc/util/object_pool/global.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc::object_pool {

struct MyNoisy {
  MyNoisy() = default;

  MyNoisy(std::string s, int d) : str(std::move(s)), data(d) {}

  ~MyNoisy() { deconstructed_nums.fetch_add(1, std::memory_order_relaxed); }

  std::string str;
  int data{0};

  static std::atomic_int32_t deconstructed_nums;
};

std::atomic_int32_t MyNoisy::deconstructed_nums = 0;

template <>
struct ObjectPoolTraits<MyNoisy> {
  static constexpr ObjectPoolType kType = ObjectPoolType::kGlobal;
};

// Used to test object pool with specified 'kMaxObjectNum'.
struct MyObject {
  int data{0};
};

// Used to test object pool without specified 'kMaxObjectNum'.
struct MyData {
  int data{0};
};

template <>
struct ObjectPoolTraits<MyObject> {
  static constexpr ObjectPoolType kType = ObjectPoolType::kGlobal;
  static constexpr size_t kMaxObjectNum = 1024;
};

template <>
struct ObjectPoolTraits<MyData> {
  static constexpr ObjectPoolType kType = ObjectPoolType::kGlobal;
};

}  // namespace trpc::object_pool

namespace trpc::testing {

using trpc::object_pool::MakeLwUnique;
using trpc::object_pool::MyData;
using trpc::object_pool::MyNoisy;
using trpc::object_pool::MyObject;

TEST(global, default_constructor) {
  auto deconstructed_nums = MyNoisy::deconstructed_nums.load(std::memory_order_relaxed);
  {
    auto object = MakeLwUnique<MyNoisy>();
    ASSERT_TRUE(object);
    EXPECT_TRUE(object->str.empty());
    EXPECT_EQ(object->data, 0);
  }
  EXPECT_EQ(deconstructed_nums + 1, MyNoisy::deconstructed_nums.load(std::memory_order_relaxed));
}

TEST(global, constructor) {
  auto deconstructed_nums = MyNoisy::deconstructed_nums.load(std::memory_order_relaxed);
  {
    auto object = MakeLwUnique<MyNoisy>("tRPC-Cpp", 930);
    ASSERT_TRUE(object);
    EXPECT_FALSE(object->str.empty());
    EXPECT_EQ(object->str, "tRPC-Cpp");
    EXPECT_EQ(object->data, 930);
  }
  EXPECT_EQ(deconstructed_nums + 1, MyNoisy::deconstructed_nums.load(std::memory_order_relaxed));
}

TEST(global, new_delete) {
  using object_pool::global::detail::kBlockChunkSize;
  using object_pool::global::detail::kBlockSize;
  using object_pool::global::detail::kFreeSlotsSize;

  std::size_t nums = kBlockSize * kBlockChunkSize + 1;

  std::vector<object_pool::LwUniquePtr<MyNoisy>> vec;
  vec.reserve(nums);
  auto deconstructed_nums = MyNoisy::deconstructed_nums.load(std::memory_order_relaxed);
  for (std::size_t i = 0; i < nums; ++i) {
    vec.emplace_back(MakeLwUnique<MyNoisy>());
  }
  vec.clear();
  EXPECT_EQ(deconstructed_nums + nums, MyNoisy::deconstructed_nums.load(std::memory_order_relaxed));

  deconstructed_nums += nums;
  nums = kFreeSlotsSize + 1;
  for (std::size_t i = 0; i < nums; ++i) {
    vec.emplace_back(MakeLwUnique<MyNoisy>());
  }
  vec.clear();
  EXPECT_EQ(deconstructed_nums + nums, MyNoisy::deconstructed_nums.load(std::memory_order_relaxed));
}

struct MyQueue {
  std::vector<object_pool::LwUniquePtr<MyNoisy>> vec;
  std::mutex mtx;
};

TEST(global, multi_thread_new_delete) {
  constexpr std::size_t kThreadNums = 4;
  constexpr std::size_t kLoopNums = kThreadNums * 30;
  std::vector<MyQueue> queue_vec(kThreadNums);
  std::vector<std::thread> thread_vec;
  thread_vec.reserve(kThreadNums);
  auto deconstructed_nums = MyNoisy::deconstructed_nums.load(std::memory_order_relaxed);
  for (std::size_t i = 0; i < kThreadNums; ++i) {
    thread_vec.emplace_back([&queue_vec, index = i, kThreadNums] {
      // poll and deliver anything to all queues
      for (std::size_t i = 0; i < kLoopNums; ++i) {
        auto& target_queue = queue_vec[i % kThreadNums];
        std::unique_lock lc(target_queue.mtx);
        target_queue.vec.emplace_back(MakeLwUnique<MyNoisy>());
      }

      auto& my_queue = queue_vec[index];
      std::size_t get_nums = 0;
      while (get_nums < kLoopNums) {
        std::unique_lock lc(my_queue.mtx);
        if (my_queue.vec.empty()) {
          lc.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          continue;
        }
        my_queue.vec.pop_back();
        lc.unlock();
        ++get_nums;
      }
    });
  }
  for (auto&& t : thread_vec) {
    t.join();
  }
  EXPECT_EQ(deconstructed_nums + (kLoopNums * kThreadNums),
            MyNoisy::deconstructed_nums.load(std::memory_order_relaxed));
}

TEST(global, new_over_limit) {
  // test object pool without specified 'kMaxObjectNum'
  {
    using object_pool::global::kDefaultMaxObjectNum;
    std::vector<MyData*> vec;
    vec.resize(kDefaultMaxObjectNum);
    for (size_t i = 0; i < kDefaultMaxObjectNum - 1; i++) {
      vec.push_back(trpc::object_pool::New<MyData>());
    }
    MyData* p = trpc::object_pool::New<MyData>();
    auto* slot = reinterpret_cast<object_pool::global::detail::Slot<MyData>*>(p);
    ASSERT_FALSE(slot->need_free_to_system);
    vec.push_back(p);

    p = trpc::object_pool::New<MyData>();
    slot = reinterpret_cast<object_pool::global::detail::Slot<MyData>*>(p);
    vec.push_back(p);
    ASSERT_TRUE(slot->need_free_to_system);

    ASSERT_TRUE(object_pool::global::GetTlsStatistics<MyData>().slot_from_system > 0);
    for (auto& t : vec) {
      trpc::object_pool::Delete<MyData>(t);
    }
    ASSERT_TRUE(object_pool::global::GetTlsStatistics<MyData>().slot_from_system == 0);
    vec.clear();
  }
  // test object pool with specified 'kMaxObjectNum'
  {
    std::vector<MyObject*> vec;
    vec.resize(object_pool::ObjectPoolTraits<MyObject>::kMaxObjectNum);
    for (size_t i = 0; i < object_pool::ObjectPoolTraits<MyObject>::kMaxObjectNum - 1; i++) {
      vec.push_back(trpc::object_pool::New<MyObject>());
    }

    MyObject* p = trpc::object_pool::New<MyObject>();
    auto* slot = reinterpret_cast<object_pool::global::detail::Slot<MyObject>*>(p);
    ASSERT_FALSE(slot->need_free_to_system);
    vec.push_back(p);

    p = trpc::object_pool::New<MyObject>();
    slot = reinterpret_cast<object_pool::global::detail::Slot<MyObject>*>(p);
    vec.push_back(p);
    ASSERT_TRUE(slot->need_free_to_system);

    ASSERT_TRUE(object_pool::global::GetTlsStatistics<MyObject>().slot_from_system > 0);
    for (auto& t : vec) {
      trpc::object_pool::Delete<MyObject>(t);
    }
    ASSERT_TRUE(object_pool::global::GetTlsStatistics<MyObject>().slot_from_system == 0);
    vec.clear();
  }
}

}  // namespace trpc::testing
