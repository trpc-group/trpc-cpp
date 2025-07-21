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

#include <deque>

#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/function.h"

namespace trpc {

namespace detail {
template <typename T>
constexpr size_t FiberQueueDefaultSize(const T&) {
  return 1;
}
}  // namespace detail

/// @brief A BoundedQueue for fiber, you can Push,Pop
/// @note  It only uses in fiber runtime.
template <typename T>
class FiberBlockingBoundedQueue {
 public:
  explicit FiberBlockingBoundedQueue(size_t capacity,
                                     Function<size_t(const T&)>&& size_func = detail::FiberQueueDefaultSize<T>)
      : size_(0), capacity_(capacity), size_func_(std::move(size_func)), stop_token_(false) {}

  FiberBlockingBoundedQueue(const FiberBlockingBoundedQueue& other) { CopyFrom(other); }
  FiberBlockingBoundedQueue(FiberBlockingBoundedQueue&& other) noexcept { MoveFrom(std::move(other)); }

  FiberBlockingBoundedQueue& operator=(const FiberBlockingBoundedQueue& other) {
    CopyFrom(other);
    return *this;
  }
  FiberBlockingBoundedQueue& operator=(FiberBlockingBoundedQueue&& other) noexcept {
    MoveFrom(std::move(other));
    return *this;
  }

  void Push(T&& item) {
    {
      size_t item_size = size_func_(item);
      std::unique_lock<FiberMutex> lk(mutex_);
      not_full_.wait(lk, [&] { return CanPush(item_size); });
      if (!stop_token_) {
        content_.push_back(std::move(item));
        size_ += item_size;
      }
    }
    not_empty_.notify_one();
  }

  template <typename Rep, typename Period>
  bool Push(T&& item, const std::chrono::duration<Rep, Period>& expires_in) {
    {
      size_t item_size = size_func_(item);
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_full_.wait_for(lk, expires_in, [&] { return CanPush(item_size); })) {
        return false;
      }
      if (!stop_token_) {
        content_.push_back(std::move(item));
        size_ += item_size;
      }
    }
    not_empty_.notify_one();
    return true;
  }

  template <typename Clock, typename Period>
  bool Push(T&& item, const std::chrono::time_point<Clock, Period>& expires_at) {
    {
      size_t item_size = size_func_(item);
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_full_.wait_until(lk, expires_at, [&] { return CanPush(item_size); })) {
        return false;
      }
      if (!stop_token_) {
        content_.push_back(std::move(item));
        size_ += item_size;
      }
    }
    not_empty_.notify_one();
    return true;
  }

  bool TryPush(T&& item) {
    {
      size_t item_size = size_func_(item);
      std::lock_guard lk(mutex_);
      if (!CanPush(item_size)) {
        return false;
      }
      if (!stop_token_) {
        content_.push_back(std::move(item));
        size_ += item_size;
      }
    }
    not_empty_.notify_one();
    return true;
  }

  void Pop(T& item) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      not_empty_.wait(lk, [this] { return CanPop(); });
      if (!stop_token_) {
        item = std::move(content_.front());
        content_.pop_front();
        size_ -= size_func_(item);
      }
    }
    not_full_.notify_one();
  }

  template <typename Rep, typename Period>
  bool Pop(T& item, const std::chrono::duration<Rep, Period>& expires_in) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_empty_.wait_for(lk, expires_in, [this] { return CanPop(); })) {
        return false;
      }
      if (!stop_token_) {
        item = std::move(content_.front());
        content_.pop_front();
        size_ -= size_func_(item);
      }
    }
    not_full_.notify_one();
    return true;
  }

  template <typename Clock, typename Period>
  bool Pop(T& item, const std::chrono::time_point<Clock, Period>& expires_at) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_empty_.wait_until(lk, expires_at, [this] { return CanPop(); })) {
        return false;
      }
      if (!stop_token_) {
        item = std::move(content_.front());
        content_.pop_front();
        size_ -= size_func_(item);
      }
    }
    not_full_.notify_one();
    return true;
  }

  bool TryPop(T& item) {
    {
      std::lock_guard lk(mutex_);
      if (!CanPop()) {
        return false;
      }
      if (!stop_token_) {
        item = std::move(content_.front());
        content_.pop_front();
        size_ -= size_func_(item);
      }
    }
    not_full_.notify_one();
    return true;
  }

  size_t Size() const { return size_; }

  size_t Capacity() const { return capacity_; }

  void SetCapacity(size_t capacity) {
    bool do_notify = capacity_ < capacity;
    capacity_ = capacity;
    if (do_notify) not_full_.notify_one();
  }

  void Stop() {
    stop_token_ = true;
    not_full_.notify_one();
    not_empty_.notify_one();
  }

 private:
  std::deque<T> content_;
  size_t size_;
  size_t capacity_;
  Function<size_t(const T&)> size_func_;

  mutable FiberMutex mutex_;
  FiberConditionVariable not_empty_;
  FiberConditionVariable not_full_;
  bool stop_token_;

  bool CanPush(size_t item_size) { return item_size + size_ <= capacity_ || stop_token_; }

  bool CanPop() { return !content_.empty() || stop_token_; }

  void CopyFrom(const FiberBlockingBoundedQueue& other) {
    if (this != &other && fiber::detail::IsFiberContextPresent()) {
      std::scoped_lock lk(mutex_, other.mutex_);
      content_ = other.content_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      size_func_ = other.size_func_;
      stop_token_ = other.stop_token_;
    }
  }

  void MoveFrom(FiberBlockingBoundedQueue&& other) noexcept {
    if (this != &other && fiber::detail::IsFiberContextPresent()) {
      std::scoped_lock lk(mutex_, other.mutex_);
      content_ = std::move(other.content_);
      size_ = other.size_;
      capacity_ = other.capacity_;
      size_func_ = std::move(other.size_func_);
      stop_token_ = other.stop_token_;
    }
  }
};

}  // namespace trpc
