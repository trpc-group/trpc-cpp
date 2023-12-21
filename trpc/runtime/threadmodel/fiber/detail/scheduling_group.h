//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/scheduling_group.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

#include "trpc/runtime/threadmodel/fiber/detail/fiber_desc.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/scheduling.h"
#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"
#include "trpc/util/align.h"
#include "trpc/util/check.h"
#include "trpc/util/function.h"

namespace trpc::fiber::detail {

/// @brief Fiber task scheduling group.
///        Each SchedulingGroup contains a group of worker threads, a timer processing thread, and a fiber task
///        scheduler. The SchedulingGroup itself is just a data structure, and the actual processing of fibers and
///        timers is done by internal FiberWorker, TimerWorker, and Scheduling.
class alignas(hardware_destructive_interference_size) SchedulingGroup {
 public:
  /// The worker index used by TimerWorker.
  inline static constexpr std::size_t kTimerWorkerIndex = -1;

  /// @brief Construct a SchedulingGroup that contains 'size' worker threads.
  /// @param affinity the CPU affinity of threads.
  /// @param size number of fiber worker threads
  /// @param scheduling_name name of scheduling
  /// @note the 'size' does not include the TimerWorker thread.
  SchedulingGroup(const std::vector<unsigned>& affinity, uint8_t size, std::string_view scheduling_name);

  ~SchedulingGroup() = default;

  /// @brief Get the SchedulingGroup of current thread
  static SchedulingGroup* Current() noexcept { return current_; }

  /// @brief Get the SchedulingGroup where the timer task is located.
  /// @param timer_id id of timer task
  static SchedulingGroup* GetTimerOwner(std::uint64_t timer_id) noexcept {
    return TimerWorker::GetTimerOwner(timer_id)->GetSchedulingGroup();
  }

  /// @brief Set up the TimerWorker. This method must be called before registering fiber worker threads.
  ///        The TimerWorker will be used later by 'SetTimer' to set timers.
  ///        The SchedulingGroup itself ensures that the thread calling this interface is one of the workers in the
  ///        SchedulingGroup. (For example, the number of threads calling TimerWorker is equal to GroupSize().)
  /// @param worker TimerWorker*
  void SetTimerWorker(TimerWorker* worker) noexcept;

  /// @brief Add an foreign scheduling group for task stealing, which needs to be called before starting the fiber
  ///        worker.
  /// @param worker_index logical id of fiber worker thread
  /// @param sg foreign scheduling group
  /// @param steal_every_n task stealing factor
  void AddForeignSchedulingGroup(std::size_t worker_index, SchedulingGroup* sg, std::uint64_t steal_every_n) noexcept;

  /// @brief Steal fiber task from foreign scheduling group
  /// @return stolen fiber task, if failed, return nullptr
  FiberEntity* StealFiberFromForeignSchedulingGroup() noexcept;

  /// @brief For foregin scheduling group to acquire fiber task in this scheduling group
  /// @return stolen fiber task, if failed, return nullptr
  FiberEntity* RemoteAcquireFiber() noexcept;

  /// @brief task scheduling action for fiber worker thread
  void Schedule() noexcept;

  /// @brief Start and run one fiber task
  bool StartFiber(FiberDesc* fiber) noexcept;

  /// @brief Schedule fibers in [start, end) to run in batch
  ///        Provided for performance. reasons. The same behavior can be achieved by calling 'StartFiber' multiple
  ///        times.
  /// @param start start the beginning range (include)
  /// @param end the ending range (exclude)
  /// @note  No scheduling lock should be held by the caller, and all fibers to be scheduled mustn't be run before
  ///        (i.e., this is the fiber time they're pushed into run queue.).
  void StartFibers(FiberDesc** start, FiberDesc** end) noexcept;

  /// @brief Suspend the execution of the current fiber, which need to be woken by someone else explicitly via 'Resume'
  /// @param self the current fiber
  /// @param scheduler_lock scheduler_lock
  /// @note The 'FiberEntity::scheduler_lock' of the fiber must be held by the caller
  ///       This behavior can help prevent a race condition between calling this method and the 'Resume' method
  void Suspend(FiberEntity* self, std::unique_lock<Spinlock>&& scheduler_lock) noexcept;

  /// @brief Resumes the execution of the fiber, used in conjunction with 'Suspend'
  /// @param to The fiber to be resumed for execution
  void Resume(FiberEntity* to) noexcept;

  /// @brief Resumes the execution of the fiber, used in conjunction with 'Suspend'
  /// @param self The currently executing fiber, will be rescheduled
  /// @param to The fiber to be resumed for execution
  void Resume(FiberEntity* self, FiberEntity* to) noexcept;

  /// @brief Yield pthread worker to someone else.
  ///        The caller must not be added to run queue by anyone else (either concurrently or prior to this call).
  ///        It will be added to run queue by this method.
  /// @param self the current fiber
  /// @note `self->scheduler_lock` must NOT be held.
  void Yield(FiberEntity* self) noexcept;

  /// @brief Yield pthread worker to the specified fiber.
  ///        Both `self` and `to` must not be added to run queue by anyone else (either concurrently or prior to this
  ///        call). They'll be add to run queue by this method.
  /// @note  Scheduler lock of `self` and `to` must NOT be held.
  /// @param self the current fiber
  /// @param to the specified fiber to be run
  void SwitchTo(FiberEntity* self, FiberEntity* to) noexcept;

  /// @brief Create a (not-scheduled-yet) timer. You must enable it later via 'EnableTimer'.
  ///
  ///        Timer ID returned is also given to timer callback on called.
  ///        Timer ID returned must be either detached via `DetachTimer` or freed (the timer itself is cancelled in if
  ///        freed) via `RemoveTimer`. Otherwise a leak will occur.
  ///
  ///        The reason why we choose a two-step way to set up a timer is that, in certain cases, the timer's callback
  ///        may want to access timer's ID stored somewhere (e.g., some global data structure). If creating timer and
  ///        enabling it is done in a single step, the user will have a hard time to synchronizes between timer-creator
  ///        and timer-callback.
  /// @param expires_at The time point at which the timer is triggered.
  /// @param cb callback function
  /// @return timer id
  /// @note  This method can only be called inside **this** scheduling group's fiber worker context.
  ///        Note that `cb` is called in timer worker's context, you normally would want to fire a fiber for run your
  ///        own logic.
  [[nodiscard]] std::uint64_t CreateTimer(std::chrono::steady_clock::time_point expires_at,
                                          Function<void(std::uint64_t)>&& cb) {
    TRPC_CHECK(timer_worker_);
    TRPC_CHECK_EQ(Current(), this);
    return timer_worker_->CreateTimer(expires_at, std::move(cb));
  }

  /// @brief Create a periodic timer.
  /// @param initial_expires_at the first time point at which the timer is triggered
  /// @param interval the execution interval
  /// @param cb callback function
  /// @return timer id
  [[nodiscard]] std::uint64_t CreateTimer(std::chrono::steady_clock::time_point initial_expires_at,
                                          std::chrono::nanoseconds interval, Function<void(std::uint64_t)>&& cb) {
    TRPC_CHECK(timer_worker_);
    TRPC_CHECK_EQ(Current(), this);
    return timer_worker_->CreateTimer(initial_expires_at, interval, std::move(cb));
  }

  /// @brief Enable a timer by timer id.
  /// @param timer_id timer id
  /// @note Timer's callback can be called even before this method returns.
  void EnableTimer(std::uint64_t timer_id) {
    TRPC_CHECK(timer_worker_);
    TRPC_CHECK_EQ(Current(), this);
    return timer_worker_->EnableTimer(timer_id);
  }

  /// @brief Detach a timer.
  /// @param timer_id timer id
  void DetachTimer(std::uint64_t timer_id) noexcept {
    TRPC_CHECK(timer_worker_);
    return timer_worker_->DetachTimer(timer_id);
  }

  /// @brief Cancel a timer set by `SetTimer`.
  ///        Callable in any thread belonging to the same scheduling group.
  /// @param timer_id timer id
  /// @note If the timer is being fired (or has fired), this method does nothing.
  void RemoveTimer(std::uint64_t timer_id) noexcept {
    TRPC_CHECK(timer_worker_);
    return timer_worker_->RemoveTimer(timer_id);
  }

  /// @brief Workers (including timer worker) call this to join this scheduling group.
  ///        Several thread-local variables used by `SchedulingGroup` is initialize here.
  ///        After calling this method, `Current` is usable.
  /// @param index index
  void EnterGroup(std::size_t index);

  /// @brief You're calling this on thread exit.
  void LeaveGroup();

  /// @brief Get number of worker threads in the SchedulingGroup (don't contains TimerWorker)
  /// @return number of worker threads
  std::size_t GroupSize() const noexcept;

  /// @brief CPU affinity of this scheduling group, or an empty vector if not specified.
  const std::vector<unsigned>& Affinity() const noexcept;

  /// @brief Stopthe scheduling group.
  ///        Wake up all workers blocking on 'WaitForFiber', once all ready fiber has terminated,
  ///        All further calls to 'AcquireFiber' returns 'kSchedulingGroupShuttingDown'.
  ///        It's still your responsibility to shut down pthread workers / timer workers. This method only mark this
  ///        control structure as being shutting down.
  void Stop();

  /// @brief Get the size of fiber task in current scheduling groups's task queue.
  std::size_t GetFiberQueueSize() const noexcept { return scheduling_->GetFiberQueueSize(); }

  /// @brief Get and set the logical id of scheduling group.
  uint8_t GetSchedulingGroupId() const { return sg_id_; }
  void SetSchedulingGroupId(uint8_t sg_id) { sg_id_ = sg_id; }

  /// @brief Get and set the logical id of the NUMA node to which it belongs.
  uint8_t GetNodeId() const { return node_id_; }
  void SetNodeId(uint8_t node_id) { node_id_ = node_id; }

  /// @brief Get and set the logical id of the thread model instance to which it belongs.
  uint8_t GetThreadModelId() const { return thread_model_id_; }
  void SetThreadModelId(uint8_t thread_model_id) { thread_model_id_ = thread_model_id; }

  /// @brief Get and set the group name of the thread model instance to which it belongs.
  std::string GetThreadModeGroupName() const { return thread_model_group_name_; }
  void SetThreadModeGroupName(const std::string& name) { thread_model_group_name_ = name; }

 private:
  static inline thread_local SchedulingGroup* current_;

  uint8_t group_size_;

  uint8_t sg_id_{0};

  uint8_t node_id_{0};

  uint8_t thread_model_id_{0};

  std::string thread_model_group_name_;

  std::vector<unsigned> affinity_;

  TimerWorker* timer_worker_{nullptr};

  std::unique_ptr<Scheduling> scheduling_{nullptr};
};

}  // namespace trpc::fiber::detail
