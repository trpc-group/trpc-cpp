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

#include <cstdint>
#include <queue>

#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"

namespace trpc {

/// @brief Timer manager, based on min-heap
/// @note  All interface not thread-safe
class TimerQueue {
 public:
  TimerQueue();

  /// @brief Create timer
  /// @param expiration The time when the timer first expires
  /// @param interval Time interval for timer execution
  /// @param executor Timer execute function
  /// @return Return the id of the timer, if the id equal UINT64_MAX, create failed
  /// @note The return timer id must be cancel by CancelTimer or detach by DetachTimer.
  ///       Otherwise, it will cause resource leak of timer
  uint64_t AddTimerAt(uint64_t expiration, uint64_t interval, Reactor::TimerExecutor&& executor);

  /// @brief Create timer
  /// @param delay The delay when the timer first expires
  /// @param interval Time interval for timer execution
  /// @param executor Timer execute function
  /// @return Return the id of the timer, if the id equal UINT64_MAX, create failed
  /// @note The return timer id must be cancel by CancelTimer or detach by DetachTimer.
  ///       Otherwise, it will cause resource leak of timer.
  uint64_t AddTimerAfter(uint64_t delay, uint64_t interval, Reactor::TimerExecutor&& executor);

  /// @brief Detach `timer_id` without actually killing the timer.
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  void DetachTimer(uint64_t timer_id);

  /// @brief Cancel timer
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  void CancelTimer(uint64_t timer_id);

  /// @brief Pause timer
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  void PauseTimer(uint64_t timer_id);

  /// @brief Resume timer
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  void ResumeTimer(uint64_t timer_id);

  /// @brief Create timer
  /// @param timer_task The timer task created by `object_pool::MakeLwShared<TimerTask>()`
  /// @note The return timer id must be cancel by CancelTimer or detach by DetachTimer.
  ///       Otherwise, it will cause resource leak of timer
  uint64_t AddTimer(TimerTask* timer_task);

  /// @brief Running time-out timers
  /// @param now_ms Current time(ms)
  void RunExpiredTimers(uint64_t now_ms);

  /// @brief Get the expiration time of the latest timer to time out
  uint64_t NextExpiration() const;

  /// @brief Get total size of timers
  uint32_t Size() const;

 private:
  struct GreaterTimerTask {
    bool operator()(const TimerTaskPtr& x, const TimerTaskPtr& y) const {
      return x->expiration > y->expiration;
    }
  };

  class TimerTaskQueue : public std::priority_queue<TimerTaskPtr, std::vector<TimerTaskPtr>,
                                                    GreaterTimerTask> {
   public:
    explicit TimerTaskQueue(size_t reserve_size) { this->c.reserve(reserve_size); }
  };

  TimerTaskQueue timer_task_queue_;
};

}  // namespace trpc
