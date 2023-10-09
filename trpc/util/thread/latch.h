//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/latch.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <condition_variable>
#include <mutex>

#include "trpc/util/check.h"

namespace trpc {

/// @brief  A downward counter of type std::ptrdiff_t which can be used to synchronize threads
/// @note: We do not perfectly match `std::latch` yet.
/// @sa: N4842, 32.8.1 Latches [thread.latch].
class Latch {
 public:
  explicit Latch(std::ptrdiff_t count);

  /// @brief Decrement internal counter. If it reaches zero, wake up all waiters.
  /// @param n the value by which the internal counter is decreased
  void count_down(std::ptrdiff_t n = 1);

  /// @brief Test if the latch's internal counter has become zero.
  /// @return bool, true: only if the internal counter has reached zero
  bool try_wait() const noexcept;

  /// @brief Wait until `Latch`'s internal counter reached zero.
  void wait() const;

  /// @brief Extension to `std::latch`.
  /// @param timeout the maximum time to spend waiting
  template <class Rep, class Period>
  bool wait_for(const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock lk(m_);
    TRPC_CHECK_GE(count_, 0);
    return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
  }

  /// @brief Extension to `std::latch`.
  /// @param timeout the time when to stop waiting
  template <class Clock, class Duration>
  bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout) {
    std::unique_lock lk(m_);
    TRPC_CHECK_GE(count_, 0);
    return cv_.wait_until(lk, timeout, [this] { return count_ == 0; });
  }

  /// @brief Shorthand for `count_down(); wait();`
  /// @param n the value by which the internal counter is decreased
  void arrive_and_wait(std::ptrdiff_t n = 1);

 private:
  mutable std::mutex m_;
  mutable std::condition_variable cv_;
  std::ptrdiff_t count_;
};

}  // namespace trpc
