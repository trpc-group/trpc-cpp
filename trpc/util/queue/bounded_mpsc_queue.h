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
#include <cstdint>
#include <memory>
#include <iostream>
#include <type_traits>

#include "trpc/util/algorithm/power_of_two.h"
#include "trpc/util/queue/detail/util.h"

namespace trpc {

/// @brief Implementation of thread-safe lock-free queue, supports multi-producer and single-consumer
///        Refer: http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
///        Paper: https://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf see details
template <typename T>
class alignas(64) BoundedMPSCQueue {
 public:
  static_assert(std::is_default_constructible_v<T>, "Type must be constructible");
  static_assert(std::is_move_constructible<T>::value, "Types must be move constructible");

  BoundedMPSCQueue() = default;

  /// @brief Initialize the queue
  /// @param size the value of size is preferably 2 to the nth power
  bool Init(size_t size) {
    capacity_ = RoundUpPowerOf2(size);

    elements_ = std::make_unique<Element[]>(capacity_);
    for (size_t i = 0; i < capacity_; ++i) {
      elements_[i].sequence = i;
    }

    enqueue_pos_.store(0, std::memory_order_relaxed);
    dequeue_pos_.store(0, std::memory_order_relaxed);

    return true;
  }

  /// @brief Push data into queue
  /// @param [in] data queue element
  /// @return true: success, false: queue full
  bool Push(T data) {
    size_t pos = 0;
    Element* elem;

    while (true) {
      pos = enqueue_pos_.load(std::memory_order_relaxed);
      elem = &elements_[pos & (capacity_ - 1)];
      size_t seq = elem->sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)pos;

      if (dif == 0) {
        if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        if (pos - dequeue_pos_.load(std::memory_order_relaxed) == capacity_) {
          return false;
        }
      }

      queue::detail::Pause();
    }

    elem->data = std::move(data);
    elem->sequence.store(pos + 1, std::memory_order_release);

    return true;
  }

  /// @brief Pop data from queue
  /// @param [out] data queue element
  /// @return true: success, false: queue empty
  bool Pop(T& data) {
    return Dequeue(&data);
  }

  /// @brief Insert data into queue(for compatible)
  /// @param [in] data queue element
  /// @return true: success, false: queue full
  bool Enqueue(T&& data) {
    size_t pos = 0;
    Element* elem;

    while (true) {
      pos = enqueue_pos_.load(std::memory_order_relaxed);
      elem = &elements_[pos & (capacity_ - 1)];
      size_t seq = elem->sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)pos;

      if (dif == 0) {
        if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        if (pos - dequeue_pos_.load(std::memory_order_relaxed) == capacity_) {
          return false;
        }
      }

      queue::detail::Pause();
    }

    elem->data = std::move(data);
    elem->sequence.store(pos + 1, std::memory_order_release);

    return true;
  }

  /// @brief Pop data from queue(for compatible)
  /// @param [out] data queue element
  /// @return true: success, false: queue empty
  // template <class = std::enable_if_t<std::is_trivial_v<T>>>
  bool Dequeue(T* data) {
    size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
    Element* elem = &elements_[pos & (capacity_ - 1)];
    size_t seq = elem->sequence.load(std::memory_order_acquire);
    intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

    if (dif == 0) {
      dequeue_pos_.store(pos + 1, std::memory_order_relaxed);
      *data = std::move(elem->data);
      elem->sequence.store(pos + capacity_, std::memory_order_release);

      return true;
    }

    return false;
  }

  /// @brief Get queue size
  size_t Size() const {
    size_t enqueue_pos = enqueue_pos_.load(std::memory_order_relaxed);
    size_t dequeue_pos = dequeue_pos_.load(std::memory_order_relaxed);
    return (enqueue_pos <= dequeue_pos ? 0 : (enqueue_pos - dequeue_pos));
  }

  /// @brief Get queue capacity
  size_t Capacity() const { return capacity_; }

 private:
  BoundedMPSCQueue(const BoundedMPSCQueue& rhs) = delete;
  BoundedMPSCQueue(BoundedMPSCQueue&& rhs) = delete;
  BoundedMPSCQueue& operator=(const BoundedMPSCQueue& rhs) = delete;
  BoundedMPSCQueue& operator=(BoundedMPSCQueue&& rhs) = delete;

  struct Element {
    T data;
    std::atomic<size_t> sequence{0};

    Element() = default;

    Element(const Element&) = delete;
    Element(Element&& rhs) : sequence(rhs.sequence.load()), data(std::move(rhs.data)) {}

    Element& operator=(const Element&) = delete;
    Element& operator=(Element&& rhs) {
      if (this != &rhs) {
        sequence = rhs.sequence.load();
        data = std::move(rhs.data);
      }
      return *this;
    }
  };

 private:
  std::size_t capacity_ = 0;

  std::unique_ptr<Element[]> elements_;

  alignas(64) std::atomic<size_t> enqueue_pos_ = 0;

  alignas(64) std::atomic<size_t> dequeue_pos_ = 0;
};

}  // namespace trpc
