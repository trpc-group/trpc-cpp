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

#pragma once

#include <stdlib.h>

#include <cstdint>
#include <utility>

#include "trpc/util/log/logging.h"

namespace trpc::container {

enum class StorageOwnership { OWNS_STORAGE, NOT_OWN_STORAGE };

template <typename T>
class BoundedQueue {
 public:
  BoundedQueue(BoundedQueue const&) = delete;

  void operator=(BoundedQueue const&) = delete;

  /// @brief You have to pass the memory for storing items at creation.
  ///        The queue contains at most mem_size/sizeof(T) items.
  BoundedQueue(void* mem, size_t mem_size, StorageOwnership ownership)
      : count_(0), cap_(mem_size / sizeof(T)), start_(0), ownership_(ownership), items_(mem) {
    TRPC_ASSERT(items_);
  }

  /// @brief Construct a queue with the given capacity.
  ///        The malloc() may fail silently, call initialized() to test validity.
  ///        of the queue.
  explicit BoundedQueue(size_t capacity)
      : count_(0),
        cap_(capacity),
        start_(0),
        ownership_(StorageOwnership::OWNS_STORAGE),
        items_(malloc(capacity * sizeof(T))) {
    TRPC_ASSERT(items_);
  }

  BoundedQueue()
      : count_(0),
        cap_(0),
        start_(0),
        ownership_(StorageOwnership::NOT_OWN_STORAGE),
        items_(nullptr) {}

  ~BoundedQueue() {
    Clear();
    if (ownership_ == StorageOwnership::OWNS_STORAGE) {
      free(items_);
      items_ = nullptr;
    }
  }

  /// @brief Push |item| into bottom side of this queue.
  ///        Returns true on success, false if queue is full.
  bool Push(const T& item) {
    if (count_ < cap_) {
      new (reinterpret_cast<T*>(items_) + Mod(start_ + count_, cap_)) T(item);
      ++count_;
      return true;
    }
    return false;
  }

  /// @brief Push |item| into bottom side of this queue. If the queue is full,
  ///        pop topmost item first.
  void ElimPush(const T& item) {
    if (count_ < cap_) {
      new (reinterpret_cast<T*>(items_) + Mod(start_ + count_, cap_)) T(item);
      ++count_;
    } else {
      (reinterpret_cast<T*>(items_))[start_] = item;
      start_ = Mod(start_ + 1, cap_);
    }
  }

  /// @brief Push a default-constructed item into bottom side of this queue.
  ///        Returns address of the item inside this queue.
  T* Push() {
    if (count_ < cap_) {
      return new (reinterpret_cast<T*>(items_) + Mod(start_ + count_++, cap_)) T();
    }
    return nullptr;
  }

  /// @brief Push |item| into top side of this queue.
  ///        Returns true on success, false if queue is full.
  bool PushTop(const T& item) {
    if (count_ < cap_) {
      start_ = start_ ? (start_ - 1) : (cap_ - 1);
      ++count_;
      new (reinterpret_cast<T*>(items_) + start_) T(item);
      return true;
    }
    return false;
  }

  /// @brief Push a default-constructed item into top side of this queue.
  ///        Returns address of the item inside this queue.
  T* PushTop() {
    if (count_ < cap_) {
      start_ = start_ ? (start_ - 1) : (cap_ - 1);
      ++count_;
      return new (reinterpret_cast<T*>(items_) + start_) T();
    }
    return nullptr;
  }

  /// @brief Pop top-most item from this queue.
  ///        Returns true on success, false if queue is empty.
  bool Pop() {
    if (count_) {
      --count_;
      (reinterpret_cast<T*>(items_) + start_)->~T();
      start_ = Mod(start_ + 1, cap_);
      return true;
    }
    return false;
  }

  /// @brief Pop top-most item from this queue and copy into |item|.
  ///        Returns true on success, false if queue is empty.
  bool Pop(T* item) {
    if (count_) {
      --count_;
      T* const p = reinterpret_cast<T*>(items_) + start_;
      *item = *p;
      p->~T();
      start_ = Mod(start_ + 1, cap_);
      return true;
    }
    return false;
  }

  /// @brief Pop bottom-most item from this queue.
  ///        Returns true on success, false if queue is empty.
  bool PopBottom() {
    if (count_) {
      --count_;
      (reinterpret_cast<T*>(items_) + Mod(start_ + count_, cap_))->~T();
      return true;
    }
    return false;
  }

  /// @brief Pop bottom-most item from this queue and copy into |item|.
  ///        Returns true on success, false if queue is empty.
  bool PopBottom(T* item) {
    if (count_) {
      --count_;
      T* const p = reinterpret_cast<T*>(items_) + Mod(start_ + count_, cap_);
      *item = *p;
      p->~T();
      return true;
    }
    return false;
  }

  /// @brief Pop all items.
  void Clear() {
    for (uint32_t i = 0; i < count_; ++i) {
      (reinterpret_cast<T*>(items_) + Mod(start_ + i, cap_))->~T();
    }
    count_ = 0;
    start_ = 0;
  }

  /// @brief Get address of top-most item, NULL if queue is empty.
  T* Top() { return count_ ? (reinterpret_cast<T*>(items_) + start_) : nullptr; }
  const T* Top() const { return count_ ? (reinterpret_cast<const T*>(items_) + start_) : nullptr; }

  /// @brief Randomly access item from top side.
  ///        top(0) == top(), top(size()-1) == bottom()
  ///        Returns NULL if |index| is out of range.
  T* Top(size_t index) {
    if (index < count_) {
      return reinterpret_cast<T*>(items_) + Mod(start_ + index, cap_);
    }
    // including _count == 0.
    return nullptr;
  }

  const T* Top(size_t index) const {
    if (index < count_) {
      return reinterpret_cast<const T*>(items_) + Mod(start_ + index, cap_);
    }
    // including _count == 0.
    return nullptr;
  }

  /// @brief Get address of bottom-most item, NULL if queue is empty.
  T* Bottom() {
    return count_ ? (reinterpret_cast<T*>(items_) + Mod(start_ + count_ - 1, cap_)) : nullptr;
  }

  const T* Bottom() const {
    return count_ ? (reinterpret_cast<const T*>(items_) + Mod(start_ + count_ - 1, cap_)) : nullptr;
  }

  /// @brief Randomly access item from bottom side.
  ///        bottom(0) == bottom(), bottom(size()-1) == top().
  ///        Returns NULL if |index| is out of range.
  T* Bottom(size_t index) {
    if (index < count_) {
      return reinterpret_cast<T*>(items_) + Mod(start_ + count_ - index - 1, cap_);
    }
    // including _count == 0.
    return nullptr;
  }

  const T* Bottom(size_t index) const {
    if (index < count_) {
      return reinterpret_cast<const T*>(items_) + Mod(start_ + count_ - index - 1, cap_);
    }
    // including _count == 0.
    return nullptr;
  }

  [[nodiscard]] bool Empty() const { return !count_; }

  [[nodiscard]] bool Full() const { return cap_ == count_; }

  /// @brief Number of items.
  [[nodiscard]] size_t Size() const { return count_; }

  /// @brief Maximum number of items that can be in this queue.
  [[nodiscard]] size_t Capacity() const { return cap_; }

  /// @brief Maximum value of capacity().
  [[nodiscard]] size_t MaxCapacity() const { return (1UL << (sizeof(cap_) * 8)) - 1; }

  /// @brief True if the queue was constructed successfully.
  [[nodiscard]] bool Initialized() const { return items_ != nullptr; }

  /// @brief internal fields with another queue.
  void Swap(BoundedQueue& rhs) {
    std::swap(count_, rhs.count_);
    std::swap(cap_, rhs.cap_);
    std::swap(start_, rhs.start_);
    std::swap(ownership_, rhs.ownership_);
    std::swap(items_, rhs.items_);
  }

 private:
  /// @brief This is faster than % in this queue because most |off| are smaller.
  ///        than |cap|. This is probably not true in other place, be careful
  ///        before you use this trick.
  static uint32_t Mod(uint32_t off, uint32_t cap) {
    while (off >= cap) {
      off -= cap;
    }
    return off;
  }

  uint32_t count_;
  uint32_t cap_;
  uint32_t start_;
  StorageOwnership ownership_;
  void* items_;
};

}  // namespace trpc::container
