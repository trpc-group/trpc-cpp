//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/condition_variable.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <condition_variable>
#include <utility>

#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"

namespace trpc {

/// @brief Adaptive condition variable primitive for both fiber and pthread context.
class FiberConditionVariable {
 public:
  /// @brief Wake up one waiter.
  void notify_one() noexcept;

  /// @brief Wake up all waiter.
  void notify_all() noexcept;

  /// @brief Wait until someone called `notify_xxx()`.
  void wait(std::unique_lock<FiberMutex>& lock);

  /// @brief Wait until `pred` is satisfied.
  template <class Predicate>
  void wait(std::unique_lock<FiberMutex>& lock, Predicate pred);

  /// @brief Wait until either someone notified us or `expires_in` has expired.
  template <class Rep, class Period>
  std::cv_status wait_for(std::unique_lock<FiberMutex>& lock,
                          const std::chrono::duration<Rep, Period>& expires_in);

  /// @brief Wait until either `pred` is satisfied or `expires_in` has expired.
  template <class Rep, class Period, class Predicate>
  bool wait_for(std::unique_lock<FiberMutex>& lock,
                const std::chrono::duration<Rep, Period>& expires_in, Predicate pred);

  /// @brief Wait until either someone notified us or `expires_at` is reached.
  template <class Clock, class Duration>
  std::cv_status wait_until(std::unique_lock<FiberMutex>& lock,
                            const std::chrono::time_point<Clock, Duration>& expires_at);

  /// @brief Wait until either `pred` is satisfied or `expires_at` is reached.
  template <class Clock, class Duration, class Pred>
  bool wait_until(std::unique_lock<FiberMutex>& lock,
                  const std::chrono::time_point<Clock, Duration>& expires_at, Pred pred);

 private:
  fiber::detail::ConditionVariable impl_;
};

template <class Predicate>
void FiberConditionVariable::wait(std::unique_lock<FiberMutex>& lock, Predicate pred) {
  impl_.wait(lock, pred);
}

template <class Rep, class Period>
std::cv_status FiberConditionVariable::wait_for(
    std::unique_lock<FiberMutex>& lock, const std::chrono::duration<Rep, Period>& expires_in) {
  auto steady_timeout = ReadSteadyClock() + expires_in;
  return impl_.wait_until(lock, steady_timeout) ? std::cv_status::no_timeout
                                                : std::cv_status::timeout;
}

template <class Rep, class Period, class Predicate>
bool FiberConditionVariable::wait_for(std::unique_lock<FiberMutex>& lock,
                                      const std::chrono::duration<Rep, Period>& expires_in,
                                      Predicate pred) {
  auto steady_timeout = ReadSteadyClock() + expires_in;
  return impl_.wait_until(lock, steady_timeout, pred);
}

template <class Clock, class Duration>
std::cv_status FiberConditionVariable::wait_until(
    std::unique_lock<FiberMutex>& lock,
    const std::chrono::time_point<Clock, Duration>& expires_at) {
  auto steady_timeout = ReadSteadyClock() + (expires_at - Clock::now());
  return impl_.wait_until(lock, steady_timeout) ? std::cv_status::no_timeout
                                                : std::cv_status::timeout;
}

template <class Clock, class Duration, class Pred>
bool FiberConditionVariable::wait_until(std::unique_lock<FiberMutex>& lock,
                                        const std::chrono::time_point<Clock, Duration>& expires_at,
                                        Pred pred) {
  auto steady_timeout = ReadSteadyClock() + (expires_at - Clock::now());
  return impl_.wait_until(lock, steady_timeout, pred);
}

}  // namespace trpc
