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

#include "trpc/coroutine/fiber_timed_mutex.h"

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"

namespace trpc {

void FiberTimedMutex::lock() {
  TRPC_DCHECK(trpc::fiber::detail::IsFiberContextPresent());
  std::unique_lock<FiberMutex> lock(mutex_);
  while (locked_) {
    cv_.wait(lock);
  }
  locked_ = true;
}

bool FiberTimedMutex::try_lock() {
  TRPC_DCHECK(trpc::fiber::detail::IsFiberContextPresent());
  std::unique_lock<FiberMutex> lock(mutex_);
  if (locked_) {
    return false;
  }
  locked_ = true;
  return true;
}

bool FiberTimedMutex::try_lock_for(std::chrono::milliseconds timeout) {
  TRPC_DCHECK(trpc::fiber::detail::IsFiberContextPresent());
  std::unique_lock<FiberMutex> lock(mutex_);
  if (locked_) {
    if (!cv_.wait_for(lock, timeout, [this] { return !locked_; })) {
      return false;
    }
  }
  locked_ = true;
  return true;
}

bool FiberTimedMutex::try_lock_until(std::chrono::steady_clock::time_point deadline) {
  TRPC_DCHECK(trpc::fiber::detail::IsFiberContextPresent());
  std::unique_lock<FiberMutex> lock(mutex_);
  if (locked_) {
    if (!cv_.wait_until(lock, deadline, [this] { return !locked_; })) {
      return false;
    }
  }
  locked_ = true;
  return true;
}

void FiberTimedMutex::unlock() {
  TRPC_DCHECK(trpc::fiber::detail::IsFiberContextPresent());
  std::unique_lock<FiberMutex> lock(mutex_);
  locked_ = false;
  cv_.notify_one();
}

}  // namespace trpc
