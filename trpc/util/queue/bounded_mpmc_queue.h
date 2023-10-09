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
#include <type_traits>

#include "trpc/util/algorithm/power_of_two.h"
#include "trpc/util/queue/detail/util.h"

namespace trpc {

/// @brief Implementation of thread-safe lock-free queue, supports multi-producer and multi-consumer
///        Refer: http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
///        Paper: https://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf see details
template <typename T>
class alignas(64) BoundedMPMCQueue {
 public:
  static_assert(std::is_default_constructible_v<T>, "Type must be constructible");
  static_assert(std::is_move_constructible<T>::value, "Types must be move constructible");

  /// @brief success
  constexpr static int kRetOk = 0;
  /// @brief queue full
  constexpr static int kRetFull = 1;
  /// @brief queue empty
  constexpr static int kRetEmpty = 2;
  /// @brief error
  constexpr static int kRetError = -1;

  BoundedMPMCQueue() = default;

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
    Element* element = nullptr;
    size_t pos = 0;

    while (true) {
      pos = enqueue_pos_.load(std::memory_order_relaxed);
      element = &elements_[pos & (capacity_ - 1)];
      size_t seq = element->sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)pos;

      if (dif == 0) {
        if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        if ((pos - dequeue_pos_.load(std::memory_order_relaxed)) == capacity_) {
          return false;
        }
      }
      queue::detail::Pause();
    }

    element->data = std::move(data);
    element->sequence.store(pos + 1, std::memory_order_release);

    return true;
  }

  /// @brief Pop data from queue
  /// @param [out] data queue element
  /// @return true: success, false: queue empty
  bool Pop(T& data) {
    return Dequeue(&data) == kRetOk;
  }

  /// @brief Insert data into queue(for compatible)
  /// @param data queue element
  /// @param block When the queue is full,
  /// if block is true, it will wait until the data is added to the queue before returning
  /// If it is false, it will return immediately
  /// @return kRetOk/kRetFull
  int Enqueue(T&& data, bool block = false) {
    Element* elem = nullptr;
    size_t pos = 0;

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
        if ((pos - dequeue_pos_.load(std::memory_order_relaxed)) == capacity_) {
          if (!block) {
            return kRetFull;
          }
        }
      }

      queue::detail::Pause();
    }

    elem->data = std::move(data);
    elem->sequence.store(pos + 1, std::memory_order_release);

    return kRetOk;
  }

  /// @brief Pop data from queue(for compatible)
  /// @param [out] data queue element
  /// @param block When the queue is empyt,
  /// if block is true, it will wait until queue has the data before returning
  /// If it is false, it will return immediately
  /// @return kRetOk/kRetEmpty
  int Dequeue(T* data, bool block = false) {
    Element* elem = nullptr;
    size_t pos = 0;

    while (true) {
      pos = dequeue_pos_.load(std::memory_order_relaxed);
      elem = &elements_[pos & (capacity_ - 1)];
      size_t seq = elem->sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

      if (dif == 0) {
        if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        if (pos - enqueue_pos_.load(std::memory_order_relaxed) == 0) {
          if (!block) {
            return kRetEmpty;
          }
        }
      }

      queue::detail::Pause();
    }

    *data = std::move(elem->data);
    elem->sequence.store(pos + capacity_, std::memory_order_release);

    return kRetOk;
  }

  /// @brief Get queue size
  uint32_t Size() const {
    size_t enqueue_pos = enqueue_pos_.load(std::memory_order_relaxed);
    size_t dequeue_pos = dequeue_pos_.load(std::memory_order_relaxed);
    return (enqueue_pos <= dequeue_pos ? 0 : (enqueue_pos - dequeue_pos));
  }

  /// @brief Get queue capacity
  uint32_t Capacity() const { return static_cast<uint32_t>(capacity_); }

 private:
  BoundedMPMCQueue(const BoundedMPMCQueue& rhs) = delete;
  BoundedMPMCQueue(BoundedMPMCQueue&& rhs) = delete;
  BoundedMPMCQueue& operator=(const BoundedMPMCQueue& rhs) = delete;
  BoundedMPMCQueue& operator=(BoundedMPMCQueue&& rhs) = delete;

  struct Element {
    std::atomic<size_t> sequence{0};
    T data;

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
  std::size_t capacity_{0};

  std::unique_ptr<Element[]> elements_;

  alignas(64) std::atomic<size_t> enqueue_pos_ = 0;

  alignas(64) std::atomic<size_t> dequeue_pos_ = 0;
};

}  // namespace trpc
