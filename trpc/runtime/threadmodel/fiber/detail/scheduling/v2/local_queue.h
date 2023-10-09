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

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "trpc/runtime/threadmodel/fiber/detail/runnable_entity.h"

namespace trpc::fiber::detail::v2 {

/// @brief Lock-free fiber running queue with single producer and multiple consumers.
/// The implementation is based on the paper "Correct and Efficient Work-Stealing for Weak Memory Models":
/// https://www.di.ens.fr/~zappa/readings/ppopp13.pdf
class LocalQueue {
 public:
  explicit LocalQueue(int64_t capacity = 1024);

  ~LocalQueue();

  /// @brief is queue empty
  /// @return true if empty, false if no empty
  bool Empty() const noexcept  {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    return b <= t;
  }

  /// @brief Get the task size in the queue 
  size_t Size() const noexcept {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    return static_cast<size_t>(b >= t ? b - t : 0);
  }

  /// @brief Get the capacity of queue
  int64_t Capacity() const noexcept {
    return array_.load(std::memory_order_relaxed)->Capacity();
  }

  /// @brief Put one element into the queue by current thread
  /// @note Only the owner of the queue can insert data into it.
  bool Push(RunnableEntity* entity);

  /// @brief Pop one element from the queue by current thread
  /// @return Return nullptr if the queue is empty
  /// @note Only the owner of the queue can pop data from it.
  RunnableEntity* Pop();

  /// @brief Steal one element from the queue by other threads
  /// @return Return nullptr if the queue is empty
  RunnableEntity* Steal();

 private:
  struct Array {
    int64_t cap;
    int64_t mod;
    std::atomic<RunnableEntity*>* storage;

    explicit Array(int64_t c)
        : cap{c},
          mod{c - 1},
          storage{new std::atomic<RunnableEntity*>[static_cast<size_t>(cap)]} {
    }

    ~Array() { delete[] storage; }

    int64_t Capacity() const noexcept { return cap; }

    void Push(int64_t i, RunnableEntity* entity) noexcept { storage[i & mod].store(entity, std::memory_order_relaxed); }

    RunnableEntity* Pop(int64_t i) noexcept { return storage[i & mod].load(std::memory_order_relaxed); }
  };

  std::atomic<int64_t> top_;
  std::atomic<int64_t> bottom_;
  std::atomic<Array*> array_;
};

}  // namespace trpc::fiber::detail::v2
