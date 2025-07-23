//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/timer.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <chrono>
#include <cstdint>
#include <utility>

#include "trpc/util/function.h"

namespace trpc {

/// @brief Set a one-shot timer.
/// @note  Timer ID returned by this method must be destroyed by `KillTimer`.
///        It only uses in fiber runtime.
[[nodiscard]] std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at,
                                          Function<void()>&& cb);
std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at,
                            Function<void(std::uint64_t)>&& cb);

/// @brief Set a periodic timer.
/// @note  Timer ID returned by this method must be destroyed by `KillTimer`.
///        It only uses in fiber runtime.
[[nodiscard]] std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at,
                                          std::chrono::nanoseconds interval, Function<void()>&& cb);
std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at,
                            std::chrono::nanoseconds interval, Function<void(std::uint64_t)>&& cb);

/// @brief Detach `timer_id` without actually killing the timer.
void DetachFiberTimer(std::uint64_t timer_id);

void SetFiberDetachedTimer(std::chrono::steady_clock::time_point at, Function<void()>&& cb);
void SetFiberDetachedTimer(std::chrono::steady_clock::time_point at,
                           std::chrono::nanoseconds interval, Function<void()>&& cb);

/// @brief Stop timer.
/// @note  You always need to call this unless the timer has been "detach"ed,
///        otherwise it's a leak.
void KillFiberTimer(std::uint64_t timer_id);

/// @brief Managing timer using RAII.
class FiberTimerKiller {
 public:
  FiberTimerKiller();
  explicit FiberTimerKiller(std::uint64_t timer_id);
  FiberTimerKiller(FiberTimerKiller&& tk) noexcept;
  FiberTimerKiller& operator=(FiberTimerKiller&& tk) noexcept;
  ~FiberTimerKiller();

  void Reset(std::uint64_t timer_id = 0);

 private:
  std::uint64_t timer_id_ = 0;
};

/// @brief Two-stage timer creation, For internal use only. DO NOT USE IT.
///
/// In certain case, you may want to store timer ID somewhere and access that ID
/// in timer callback. Without this two-stage procedure, you need to synchronizes
/// between timer-id-filling and timer-callback.
///
/// Timer ID returned must be detached or killed. Otherwise a leak will occur.
///
[[nodiscard]] std::uint64_t CreateFiberTimer(std::chrono::steady_clock::time_point at,
                                             Function<void(std::uint64_t)>&& cb);
[[nodiscard]] std::uint64_t CreateFiberTimer(std::chrono::steady_clock::time_point at,
                                             std::chrono::nanoseconds interval,
                                             Function<void(std::uint64_t)>&& cb);

/// @brief Enable timer previously created via `CreateFiberTimer`.
/// @note  For internal use only. DO NOT USE IT.
void EnableFiberTimer(std::uint64_t timer_id);

}  // namespace trpc
