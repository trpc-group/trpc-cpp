//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/latch.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Adaptive latch primitive for both fiber and pthread context.
class FiberLatch {
 public:
  explicit FiberLatch(std::ptrdiff_t count);

  /// @brief Count the latch down.
  ///        If total number of call to this method reached `count` (passed to
  //         constructor), all waiters are waken up.
  void CountDown(std::ptrdiff_t update = 1);

  /// @brief Test if the latch's internal counter has become zero.
  bool TryWait() const noexcept;

  /// @brief Wait until the latch's internal counter becomes zero.
  void Wait() const;

  /// @brief Count the latch down and wait for it to become zero.
  ///        If total number of call to this method reached `count` (passed to
  ///        constructor), all waiters are waken up.
  void ArriveAndWait(std::ptrdiff_t update = 1);

  /// @brief Wait until the latch's internal counter becomes zero or timeout
  /// @return Return true if internal counter becomes zero without timeout, otherwise return false.
  template <class Rep, class Period>
  bool WaitFor(const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock lk(lock_);
    return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
  }

  /// @brief Wait until the latch's internal counter becomes zero or timeout
  /// @return Return true if internal counter becomes zero without timeout, otherwise return false.
  template <class Clock, class Duration>
  bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout) {
    std::unique_lock lk(lock_);
    return cv_.wait_until(lk, timeout, [this] { return count_ == 0; });
  }

 private:
  mutable FiberMutex lock_;
  mutable FiberConditionVariable cv_;
  std::ptrdiff_t count_;
};

}  // namespace trpc
