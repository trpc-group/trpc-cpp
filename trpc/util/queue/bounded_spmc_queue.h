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
#include <memory>
#include <type_traits>

#include "trpc/util/algorithm/power_of_two.h"

namespace trpc {

/// @brief Implementation of thread-safe lock-free queue, supports single-producer and multi-consumer
///        Paper: the paper "Correct and Efficient Work-Stealing for Weak Memory Models"
///        see details: https://www.di.ens.fr/~zappa/readings/ppopp13.pdf
template <typename T>
class alignas(64) BoundedSPMCQueue {
 public:
  static_assert(std::is_pointer_v<T>, "T must be a pointer type");

  explicit BoundedSPMCQueue(size_t capacity = 1024) {
    top_.store(0, std::memory_order_relaxed);
    bottom_.store(0, std::memory_order_relaxed);

    capacity_ = RoundUpPowerOf2(capacity);

    elements_ = std::make_unique<Element[]>(capacity_);
  }

  /// @brief Push data into queue, only the owner of the current thread can insert
  /// @param [in] data queue element, T is point type
  /// @return true: success, false: queue full
  bool Enqueue(T data) {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);

    if (capacity_ - 1 < (b - t)) {
      return false;
    }

    Element* element = &elements_[b & (capacity_ - 1)];
    element->data.store(data, std::memory_order_relaxed);

    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_relaxed);

    return true;
  }

  /// @brief Pop data from queue, only the owner of the current thread can pop
  /// @return Return nullptr when queue empty
  T Dequeue() {
    size_t b = bottom_.load(std::memory_order_relaxed) - 1;
    bottom_.store(b, std::memory_order_relaxed);

    std::atomic_thread_fence(std::memory_order_seq_cst);
    size_t t = top_.load(std::memory_order_relaxed);

    T data{nullptr};

    if (t <= b) {
      Element* element = &elements_[b & (capacity_ - 1)];
      data = element->data.load(std::memory_order_relaxed);
      if (t == b) {
        if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
          return nullptr;
        }
        bottom_.store(b + 1, std::memory_order_relaxed);
      }
    } else {
      bottom_.store(b + 1, std::memory_order_relaxed);
    }

    return data;
  }

  /// @brief Steal data from queue
  /// @return Return nullptr when queue empty
  T Steal() {
    size_t t = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    size_t b = bottom_.load(std::memory_order_acquire);

    T data{nullptr};

    if (t < b) {
      Element* element = &elements_[t & (capacity_ - 1)];
      data = element->data.load(std::memory_order_relaxed);
      if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        return nullptr;
      }
    }

    return data;
  }

  /// @brief Get queue size
  size_t Size() const {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_relaxed);
    return static_cast<size_t>(b >= t ? b - t : 0);
  }

  /// @brief Get queue capacity
  size_t Capacity() const { return capacity_; }

  /// @brief Is empty
  bool Empty() const {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_relaxed);
    return b <= t;
  }

 private:
  struct Element {
    std::atomic<T> data{nullptr};
  };

 private:
  std::size_t capacity_{0};

  std::unique_ptr<Element[]> elements_;

  alignas(64) std::atomic<size_t> top_;

  alignas(64) std::atomic<size_t> bottom_;
};

}  // namespace trpc
