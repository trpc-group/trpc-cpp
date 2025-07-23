//
//
// Copyright (c) 2018-2023 Dr. Tsung-Wei Huang
// taskflow is licensed under the MIT License.
// The source codes in this file based on
// https://github.com/taskflow/taskflow/blob/master/taskflow/core/tsq.hpp.
// This source file may have been modified by Tencent, and licensed under the MIT License.
//
//

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace trpc {

/// @brief Lock-free unbounded single-producer multiple-consumer queue.
/// @tparam T data type (must be a pointer type)

/// This class implements the work stealing queue described in the paper,
/// "Correct and Efficient Work-Stealing for Weak Memory Models,"
/// available at https://www.di.ens.fr/~zappa/readings/ppopp13.pdf.

/// Only the queue owner can perform pop and push operations,
/// while others can steal data from the queue.
template <typename T>
class alignas(64) UnboundedSPMCQueue {
 public:
  static_assert(std::is_pointer_v<T>, "T must be a pointer type");

  explicit UnboundedSPMCQueue(int64_t capacity = 1024);

  ~UnboundedSPMCQueue();

  /// @brief Is empty
  bool Empty() const noexcept;

  /// @brief Get queue size
  size_t Size() const noexcept;

  /// @brief Get queue capacity
  int64_t Capacity() const noexcept;

  /// @brief Push data into queue, only the owner of the current thread can insert
  /// @param [in] item queue element, T is point type
  void Push(T item);

  /// @brief Pop data from queue, only the owner of the current thread can pop
  /// @return return nullptr when queue empty
  T Pop();

  /// @brief Steal data from queue
  /// @return return nullptr when queue empty
  T Steal();

 private:
  struct alignas(64) Array {
    int64_t cap;
    int64_t mod;
    std::atomic<T>* storage;

    explicit Array(int64_t c) : cap{c}, mod{c - 1}, storage{new std::atomic<T>[static_cast<size_t>(cap)]} {}

    ~Array() { delete[] storage; }

    int64_t Capacity() const noexcept { return cap; }

    void Push(int64_t i, T o) noexcept { storage[i & mod].store(o, std::memory_order_relaxed); }

    T Pop(int64_t i) noexcept { return storage[i & mod].load(std::memory_order_relaxed); }

    Array* Resize(int64_t bottom, int64_t top) {
      Array* ptr = new Array{2 * cap};
      for (int64_t i = top; i != bottom; ++i) {
        ptr->Push(i, Pop(i));
      }
      return ptr;
    }
  };

  alignas(64) std::atomic<int64_t> top_;
  alignas(64) std::atomic<int64_t> bottom_;
  alignas(64) std::atomic<Array*> array_;
  std::vector<Array*> garbage_;
};

template <typename T>
UnboundedSPMCQueue<T>::UnboundedSPMCQueue(int64_t c) {
  assert(c && (!(c & (c - 1))));
  top_.store(0, std::memory_order_relaxed);
  bottom_.store(0, std::memory_order_relaxed);
  array_.store(new Array{c}, std::memory_order_relaxed);
  garbage_.reserve(32);
}

template <typename T>
UnboundedSPMCQueue<T>::~UnboundedSPMCQueue() {
  for (auto a : garbage_) {
    delete a;
  }
  delete array_.load();
}

template <typename T>
bool UnboundedSPMCQueue<T>::Empty() const noexcept {
  int64_t b = bottom_.load(std::memory_order_relaxed);
  int64_t t = top_.load(std::memory_order_relaxed);
  return b <= t;
}

template <typename T>
size_t UnboundedSPMCQueue<T>::Size() const noexcept {
  int64_t b = bottom_.load(std::memory_order_relaxed);
  int64_t t = top_.load(std::memory_order_relaxed);
  return static_cast<size_t>(b >= t ? b - t : 0);
}

template <typename T>
void UnboundedSPMCQueue<T>::Push(T o) {
  int64_t b = bottom_.load(std::memory_order_relaxed);
  int64_t t = top_.load(std::memory_order_acquire);
  Array* a = array_.load(std::memory_order_relaxed);

  if (a->Capacity() - 1 < (b - t)) {
    Array* tmp = a->Resize(b, t);
    garbage_.push_back(a);
    std::swap(a, tmp);
    array_.store(a, std::memory_order_release);
  }

  a->Push(b, o);
  std::atomic_thread_fence(std::memory_order_release);
  bottom_.store(b + 1, std::memory_order_relaxed);
}

template <typename T>
T UnboundedSPMCQueue<T>::Pop() {
  int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
  Array* a = array_.load(std::memory_order_relaxed);
  bottom_.store(b, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  int64_t t = top_.load(std::memory_order_relaxed);

  T item{nullptr};

  if (t <= b) {
    item = a->Pop(b);
    if (t == b) {
      // the last item just got stolen
      if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        item = nullptr;
      }
      bottom_.store(b + 1, std::memory_order_relaxed);
    }
  } else {
    bottom_.store(b + 1, std::memory_order_relaxed);
  }

  return item;
}

template <typename T>
T UnboundedSPMCQueue<T>::Steal() {
  int64_t t = top_.load(std::memory_order_acquire);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  int64_t b = bottom_.load(std::memory_order_acquire);

  T item{nullptr};

  if (t < b) {
    Array* a = array_.load(std::memory_order_consume);
    item = a->Pop(t);
    if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
      return nullptr;
    }
  }

  return item;
}

template <typename T>
int64_t UnboundedSPMCQueue<T>::Capacity() const noexcept {
  return array_.load(std::memory_order_relaxed)->Capacity();
}

}  // namespace trpc
