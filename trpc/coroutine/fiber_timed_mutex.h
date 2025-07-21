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

#include <chrono>

#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"

namespace trpc {

/// @brief Implements `std::timed_mutex` alternative for fiber.
class FiberTimedMutex {
 public:
  /// @brief Lock the mutex. If the mutex is already locked, the calling fiber will be suspended until the lock
  /// is acquired.
  void lock();

  /// @brief Try to lock the mutex. Returns true if the lock was acquired, false if the lock was not acquired
  bool try_lock();

  /// @brief Try to lock the mutex. Returns true if the lock was acquired, false if the timeout expired.
  bool try_lock_for(std::chrono::milliseconds timeout);

  /// @brief Try to lock the mutex. Returns true if the lock was acquired, false if the deadline expired.
  bool try_lock_until(std::chrono::steady_clock::time_point deadline);

  /// @brief Unlock the mutex. If there are fibers waiting for the lock, the lock will be transferred to one of them.
  void unlock();

 private:
  FiberMutex mutex_;
  FiberConditionVariable cv_;
  bool locked_ = false;
};

}  // namespace trpc
