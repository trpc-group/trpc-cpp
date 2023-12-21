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
/// reference implementation:
/// "Correct and Efficient Work-Stealing for Weak Memory Models" https://www.di.ens.fr/~zappa/readings/ppopp13.pdf
/// https://github.com/apache/incubator-brpc/blob/master/src/bthread/work_stealing_queue.h
class LocalQueue {
 public:
  LocalQueue();
  ~LocalQueue();

  bool Init(size_t capacity);

  /// @brief Check queue is empty
  /// @return true if empty, false if no empty
  bool Empty() const noexcept  {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    return b <= t;
  }

  /// @brief Get the fiber size in the queue
  size_t Size() const noexcept {
    int64_t b = bottom_.load(std::memory_order_relaxed);
    int64_t t = top_.load(std::memory_order_relaxed);
    return static_cast<size_t>(b > t ? b - t : 0);
  }

  /// @brief Get the capacity of queue
  size_t Capacity() const noexcept {
    return capacity_;
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
  std::atomic<size_t> bottom_;
  size_t capacity_;
  RunnableEntity** runnables_;
  std::atomic<size_t> top_;
};

}  // namespace trpc::fiber::detail::v2
