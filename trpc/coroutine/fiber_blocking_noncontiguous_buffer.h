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

#include <deque>

#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief A NoncontiguousBuffer for fiber, you can Append,Cut
/// @note  It only uses in fiber runtime.
class FiberBlockingNoncontiguousBuffer {
 public:
  explicit FiberBlockingNoncontiguousBuffer(size_t capacity) : capacity_(capacity), stop_token_(false) {}

  void Append(NoncontiguousBuffer&& item) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      not_full_.wait(lk, [&] { return CanAppend(item.ByteSize()); });
      content_.Append(std::move(item));
    }
    not_empty_.notify_one();
  }

  void AppendAlways(NoncontiguousBuffer&& item) {
    while (!item.Empty()) {
      {
        std::unique_lock<FiberMutex> lk(mutex_);
        not_full_.wait(lk, [&] { return content_.ByteSize() < capacity_; });
        content_.Append(item.Cut(std::min(item.ByteSize(), capacity_ - content_.ByteSize())));
      }
      not_empty_.notify_one();
    }
  }

  template <typename Rep, typename Period>
  bool Append(NoncontiguousBuffer&& item, const std::chrono::duration<Rep, Period>& expires_in) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_full_.wait_for(lk, expires_in, [&] { return CanAppend(item.ByteSize()); })) {
        return false;
      }
      content_.Append(std::move(item));
    }
    not_empty_.notify_one();
    return true;
  }

  template <typename Clock, typename Period>
  bool Append(NoncontiguousBuffer&& item, const std::chrono::time_point<Clock, Period>& expires_at) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_full_.wait_until(lk, expires_at, [&] { return CanAppend(item.ByteSize()); })) {
        return false;
      }
      content_.Append(std::move(item));
    }
    not_empty_.notify_one();
    return true;
  }

  bool TryAppend(NoncontiguousBuffer&& item) {
    {
      std::lock_guard lk(mutex_);
      if (!CanAppend(item.ByteSize())) {
        return false;
      }
      content_.Append(std::move(item));
    }
    not_empty_.notify_one();
    return true;
  }

  void Cut(NoncontiguousBuffer& item, size_t cut_size) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      not_empty_.wait(lk, [&] { return CanCut(cut_size); });
      if (!stop_token_) {
        item = content_.Cut(cut_size);
      }
    }
    not_full_.notify_one();
  }

  template <typename Rep, typename Period>
  bool Cut(NoncontiguousBuffer& item, size_t cut_size, const std::chrono::duration<Rep, Period>& expires_in) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_empty_.wait_for(lk, expires_in, [&] { return CanCut(cut_size); })) {
        return false;
      }
      item = content_.Cut(std::min(content_.ByteSize(), cut_size));
    }
    not_full_.notify_one();
    return true;
  }

  template <typename Clock, typename Period>
  bool Cut(NoncontiguousBuffer& item, size_t cut_size, const std::chrono::time_point<Clock, Period>& expires_at) {
    {
      std::unique_lock<FiberMutex> lk(mutex_);
      if (!not_empty_.wait_until(lk, expires_at, [&] { return CanCut(cut_size); })) {
        return false;
      }
      item = content_.Cut(std::min(content_.ByteSize(), cut_size));
    }
    not_full_.notify_one();
    return true;
  }

  bool TryCut(NoncontiguousBuffer& item, size_t cut_size) {
    {
      std::lock_guard lk(mutex_);
      if (!CanCut(cut_size)) {
        return false;
      }
      item = content_.Cut(std::min(content_.ByteSize(), cut_size));
    }
    not_full_.notify_one();
    return true;
  }

  size_t ByteSize() const { return content_.ByteSize(); }

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
  NoncontiguousBuffer content_;
  size_t capacity_;

  mutable FiberMutex mutex_;
  FiberConditionVariable not_empty_;
  FiberConditionVariable not_full_;
  bool stop_token_;

  bool CanAppend(size_t append_size) { return append_size + content_.ByteSize() <= capacity_ || stop_token_; }

  bool CanCut(size_t cut_size) { return cut_size <= content_.ByteSize() || stop_token_; }
};

}  // namespace trpc
