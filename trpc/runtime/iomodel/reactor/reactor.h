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

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "trpc/runtime/iomodel/reactor/event_handler.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"
#include "trpc/util/function.h"

namespace trpc {

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
class AsyncIO;
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

/// @brief Base class for reactor
class Reactor {
 public:
  /// @brief The function for common task that framework or business submited
  using Task = Function<void()>;

  /// @brief The function for timer task
  using TimerExecutor = Function<void()>;

  /// @brief Task priority
  enum class Priority {
    kLowest = 0,
    kLow,
    kNormal,
    kHigh,
  };

  static constexpr int kPriorities = 4;

 public:
  virtual ~Reactor() = default;

  /// @brief Get the tls reactor pointer
  static Reactor* GetCurrentTlsReactor() { return current_reactor; }

  /// @brief Set the tls reactor pointer
  static void SetCurrentTlsReactor(Reactor* current) { current_reactor = current; }

  /// @brief Get the unique id of the current reactor instance,
  virtual uint32_t Id() const = 0;

  /// @brief Get reactor name
  virtual std::string_view Name() const = 0;

  /// @brief Initilize
  virtual bool Initialize() = 0;

  /// @brief Start running
  virtual void Run() = 0;

  /// @brief Stop running
  virtual void Stop() = 0;

  /// @brief Wait finish
  virtual void Join() = 0;

  /// @brief Destroy resource
  virtual void Destroy() = 0;

  /// @brief Add/Delete/Modify the EventHandler in epoll
  virtual void Update(EventHandler* event_handler) = 0;

  /// @brief Submit task to the reactor(thread safe), the difference with SubmitTask2
  ///        is that this interface always puts task in the queue
  /// @param task Execute task
  /// @param priority Prority
  /// @return bool true: success, false: failed
  virtual bool SubmitTask(Task&& task, Priority priority = Priority::kNormal) = 0;

  /// @brief Submit task to the reactor(thread safe), if the current thread is the reactor owner,
  ///        the task will run directly, otherwise put task in the queue first
  /// @param task Execute task
  /// @param priority Prority
  /// @return bool true: success, false: failed
  virtual bool SubmitTask2(Task&& task, Priority priority = Priority::kNormal) = 0;

  /// @brief Submit framework inner task to the reactor task queue(thread safe)
  /// @param task Execute task
  /// @return bool true: success, false: failed
  /// @note Only use by framework
  virtual bool SubmitInnerTask(Task&& task) { return false; }

  /// @brief Add timer task(not thread-safe)
  /// @param timer_task The timer task created by `object_pool::MakeLwShared<TimerTask>()`
  /// @return Return the id of the timer, if the id equal UINT64_MAX, create failed
  /// @note The return timer id must be cancel by CancelTimer or detach by DetachTimer
  ///       Otherwise, it will cause resource leak of timer
  virtual uint64_t AddTimer(TimerTask* timer_task) { return kInvalidTimerId; }

  /// @brief Add timer task(not thread-safe)
  /// @param expiration Expiration time of timer task
  /// @param interval Interval time of timer task
  /// @param executor Execute task
  /// @return Return the id of the timer, if the id equal UINT64_MAX, create failed
  /// @note The return timer id must be cancel by CancelTimer or detach by DetachTimer
  ///       Otherwise, it will cause resource leak of timer
  virtual uint64_t AddTimerAt(uint64_t expiration, uint64_t interval, TimerExecutor&& executor) {
    return kInvalidTimerId;
  }

  /// @brief Add timer task(not thread-safe)
  /// @param delay Delay time of timer task
  /// @param interval Interval time of timer task
  /// @param executor Execute task
  /// @return Return the id of the timer, if the id equal UINT64_MAX, create failed
  /// @note The return timer id must be cancel by CancelTimer or detach by DetachTimer.
  ///       Otherwise, it will cause resource leak of timer
  virtual uint64_t AddTimerAfter(uint64_t delay, uint64_t interval, TimerExecutor&& executor) {
    return kInvalidTimerId;
  }

  /// @brief Detach `timer_id` without actually killing the timer.
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  virtual void DetachTimer(uint64_t timer_id) {}

  /// @brief Cancel timer task(not thread-safe)
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  virtual void CancelTimer(uint64_t timer_id) {}

  /// @brief Pause timer task(not thread-safe)
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  virtual void PauseTimer(uint64_t timer_id) {}

  /// @brief Resume timer task(not thread-safe)
  /// @param timer_id It created by AddTimerAt/AddTimerAfter
  virtual void ResumeTimer(uint64_t timer_id) {}

  /// @brief Get the queue size of reactor pending tasks
  virtual size_t GetTaskSize() { return 0; }

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
  virtual AsyncIO* GetAsyncIO() const { return nullptr; }
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

 private:
  static inline thread_local Reactor* current_reactor = nullptr;
};

}  // namespace trpc
