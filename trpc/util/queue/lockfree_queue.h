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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <atomic>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#include "trpc/util/algorithm/power_of_two.h"

namespace trpc {

/// @brief Implementation of a thread-safe lock-free queue that supports multiple producers and multiple
///        consumers (multiple threads simultaneously fetching/putting data).
/// @sa http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
/// @sa paper: https://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf
/// @note Now has Deprecated
template <typename T>
class alignas(64) LockFreeQueue {
 public:
  enum {
    RT_OK = 0,     ///< successfully added to the queue.
    RT_FULL = 1,   ///< the queue is full
    RT_EMPTY = 2,  ///< the queue is empty
    RT_ERR = -1,   ///< an error has occurred.
  };

  /// @brief Constructor of a lock-free queue
  /// @throw std::invalid_argument If the length of the queue is not a power of 2
  LockFreeQueue() : init_(false) {}

  ~LockFreeQueue() {}

  /// @brief Initialize queue
  /// @param size The length of the queue must be a power of 2 in order to use bitwise operations with the & operator.
  /// @return enum
  int Init(size_t size) {
    if (init_) {
      return RT_OK;
    }

    capacity_ = RoundUpPowerOf2(size);
    mask_ = capacity_ - 1;

    elements_ = std::make_unique<Element[]>(capacity_);
    for (size_t i = 0; i < capacity_; ++i) {
      elements_[i].sequence = i;
    }

    count_.store(0, std::memory_order_relaxed);

    enqueue_pos_.store(0, std::memory_order_relaxed);
    dequeue_pos_.store(0, std::memory_order_relaxed);

    return RT_OK;
  }

  ///  @brief Add the data to the queue
  ///  @param val data to be added
  ///  @return RT_OK/RT_FULL
  int Enqueue(const T& val) {
    Element* elem = nullptr;
    size_t pos = enqueue_pos_.load(std::memory_order_relaxed);

    while (true) {
      elem = &elements_[pos & mask_];
      size_t seq = elem->sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)pos;

      if (dif == 0) {
        if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        if (pos - dequeue_pos_.load(std::memory_order_relaxed) == capacity_) return RT_FULL;  // queue full

        pos = enqueue_pos_.load(std::memory_order_relaxed);
      } else {
        pos = enqueue_pos_.load(std::memory_order_relaxed);
      }
    }

    elem->data = val;
    elem->sequence.store(pos + 1, std::memory_order_release);

    count_.fetch_add(1, std::memory_order_release);

    return RT_OK;
  }

  /// @brief Retrieve data from the queue and store it in a variable called `data`.
  /// @param val data that was retrieved from the queue
  /// @return RT_OK/RT_EMPTY
  int Dequeue(T& val) {
    Element* elem = nullptr;
    size_t pos = dequeue_pos_.load(std::memory_order_relaxed);

    while (true) {
      elem = &elements_[pos & mask_];
      size_t seq = elem->sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

      if (dif == 0) {
        if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else if (dif < 0) {
        if (pos - enqueue_pos_.load(std::memory_order_relaxed) == 0) return RT_EMPTY;  // queue empty

        pos = dequeue_pos_.load(std::memory_order_relaxed);
      } else {
        pos = dequeue_pos_.load(std::memory_order_relaxed);
      }
    }

    val = elem->data;
    elem->sequence.store(pos + mask_ + 1, std::memory_order_release);

    count_.fetch_sub(1, std::memory_order_release);

    return RT_OK;
  }

  /// @brief The size of the queue
  /// @return uint32_t
  uint32_t Size() const { return count_.load(std::memory_order_relaxed); }

  /// @brief The capacity of the queue
  /// @return uint32_t
  uint32_t Capacity() const { return static_cast<uint32_t>(capacity_); }

 private:
  LockFreeQueue(const LockFreeQueue& rhs) = delete;
  LockFreeQueue(LockFreeQueue&& rhs) = delete;

  LockFreeQueue& operator=(const LockFreeQueue& rhs) = delete;
  LockFreeQueue& operator=(LockFreeQueue&& rhs) = delete;

 private:
  static constexpr std::size_t hardware_destructive_interference_size = 64;

  struct alignas(hardware_destructive_interference_size) Element {
    std::atomic<size_t> sequence;
    T data;
  };

  bool init_;

  std::size_t mask_;

  std::size_t capacity_{0};

  std::unique_ptr<Element[]> elements_;

  std::atomic<int> count_;

  alignas(hardware_destructive_interference_size) std::atomic<size_t> enqueue_pos_;

  alignas(hardware_destructive_interference_size) std::atomic<size_t> dequeue_pos_;
};

}  // namespace trpc
