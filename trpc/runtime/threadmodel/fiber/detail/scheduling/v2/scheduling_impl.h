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

#include <atomic>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/scheduling.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v1/run_queue.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v2/local_queue.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/util/align.h"
#include "trpc/util/check.h"
#include "trpc/util/delayed_init.h"
#include "trpc/util/function.h"
#include "trpc/util/thread/predicate_notifier.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::fiber::detail::v2 {

/// @brief An implementation of scheduling based on scheduling algorithm of Taskflow runtime. Relevant information can
///        be found at https://github.com/taskflow/taskflow/blob/master/taskflow/core/executor.hpp.
///        The implementation principle and advantages are as follows:
///        1. The task queue consists of a global (MPMC) and local (SPMC) queue, and supporting task stealing between
///        global and local queues.
///        2. When executing parallel tasks, the worker thread can add tasks to its local queue without notification,
///        avoiding system call overhead.
///        3. Taskflow's native notifier is used for inter-thread communication, reducing notification overhead.
///        4. In Taskflow's task scheduling, tasks cannot block the thread during execution. Once this happens, one of
///        the threads will consume 100% of the CPU. However, when the fiber reactor executes epoll_wait and there are
///        no network events, it may block the thread. To solve this problem, we treat the reactor fiber as an special
///        type fiber and put it into the separate queue to avoid frequent detection of reactor tasks in the running
///        queue, which would cause the worker thread to be unable to sleep and result in 100% CPU usage.
class alignas(hardware_destructive_interference_size) SchedulingImpl final : public trpc::fiber::detail::Scheduling {
 public:
  bool Init(SchedulingGroup* scheduling_group, std::size_t scheduling_group_size) noexcept override;

  void Enter(std::size_t index) noexcept override;

  void Schedule() noexcept override;

  void Leave() noexcept override;

  bool StartFiber(FiberDesc* fiber) noexcept override;

  void StartFibers(FiberDesc** start, FiberDesc** end) noexcept override;

  void Suspend(FiberEntity* self, std::unique_lock<Spinlock>&& scheduler_lock) noexcept override;

  void Resume(FiberEntity* to) noexcept override;

  void Resume(FiberEntity* self, FiberEntity* to) noexcept override;

  void Yield(FiberEntity* self) noexcept override;

  void SwitchTo(FiberEntity* self, FiberEntity* to) noexcept override;

  void Stop() noexcept override;

  std::size_t GetFiberQueueSize() noexcept override;

 private:
  FiberEntity* WaitForFiber() noexcept;
  FiberEntity* ExploreTask() noexcept;
  bool PushToLocalQueue(RunnableEntity* fiber) noexcept;
  bool PushToGlobalQueue(detail::v1::RunQueue& global_queue, RunnableEntity* fiber, bool wait = false) noexcept;
  bool QueueRunnableEntity(RunnableEntity* entity, bool is_fiber_reactor) noexcept;
  FiberEntity* AcquireReactorFiber() noexcept;
  FiberEntity* GetFiberEntity() noexcept;
  void Resume(FiberEntity* fiber, std::unique_lock<Spinlock>&& scheduler_lock) noexcept;
  inline FiberEntity* GetOrInstantiateFiber(RunnableEntity* entity) noexcept;
  inline void SetFiberRunning(FiberEntity* fiber_entity) noexcept;

 private:
  static constexpr auto kUninitializedWorkerIndex = std::numeric_limits<std::size_t>::max();
  static thread_local std::size_t worker_index_;

  trpc::fiber::detail::SchedulingGroup* scheduling_group_;
  std::size_t group_size_;

  std::unique_ptr<PredicateNotifier> notifier_;
  std::vector<PredicateNotifier::Waiter*> waiters_;

  // the queue used to store fiber reactor tasks
  // GlobalQueue fiber_reactor_queue_;
  v1::RunQueue fiber_reactor_queue_;
  // global queue used to store fiber tasks created by non-fiber worker threads.
  // GlobalQueue global_queue_;
  v1::RunQueue global_queue_;
  // local queue of each fiber worker thread
  std::unique_ptr<LocalQueue[]> local_queues_;

  std::atomic<std::size_t> num_actives_{0};
  std::atomic<std::size_t> num_thieves_{0};
  std::vector<std::size_t> vtm_;

  std::atomic<bool> stopped_{false};
};

}  // namespace trpc::fiber::detail::v2
