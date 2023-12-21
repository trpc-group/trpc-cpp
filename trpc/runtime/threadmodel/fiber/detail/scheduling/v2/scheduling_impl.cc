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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. taskflow
// Copyright (c) 2018-2023 Dr. Tsung-Wei Huang
// taskflow is licensed under the MIT License.
//

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v2/scheduling_impl.h"

#include <climits>
#include <memory>
#include <string>
#include <thread>

#include "trpc/runtime/threadmodel/fiber/detail/assembly.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/scheduling_var.h"
#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"
#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"
#include "trpc/util/chrono/tsc.h"
#include "trpc/util/deferred.h"
#include "trpc/util/latch.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"
#include "trpc/util/string_helper.h"

using namespace std::literals;

namespace trpc::fiber::detail::v2 {

FiberEntity* kSchedulingGroupShuttingDown = reinterpret_cast<FiberEntity*>(0x1);

thread_local std::size_t SchedulingImpl::worker_index_ = kUninitializedWorkerIndex;

bool SchedulingImpl::Init(SchedulingGroup* scheduling_group, std::size_t scheduling_group_size) noexcept {
  TRPC_CHECK_LE(scheduling_group_size, std::size_t(64),
                "We only support up to 64 workers in each scheduling group. "
                "Use more scheduling groups if you want more concurrency.");
  scheduling_group_ = scheduling_group;
  group_size_ = scheduling_group_size;
  notifier_ = std::make_unique<PredicateNotifier>(group_size_);

  // The reactor num under each scheduling group is always less than or equal to group_size_, so the size of the fiber
  // reactor queue is set to group_size.
  TRPC_ASSERT(fiber_reactor_queue_.Init(group_size_));
  TRPC_ASSERT(global_queue_.Init(GetFiberRunQueueSize()));

  local_queues_ = std::make_unique<LocalQueue[]>(group_size_);
  for (size_t i = 0; i < group_size_; ++i) {
    TRPC_ASSERT(local_queues_[i].Init(1024));
  }

  vtm_.assign(group_size_, 0);

  waiters_.resize(group_size_);
  for (std::size_t i = 0; i < group_size_; i++) {
    waiters_[i] = &notifier_->GetWaiter(i);
  }

  return true;
}

void SchedulingImpl::Enter(std::size_t index) noexcept {
  worker_index_ = index;
  if (worker_index_ < group_size_) {
    vtm_[worker_index_] = worker_index_;
  }

  if (worker_index_ != SchedulingGroup::kTimerWorkerIndex) {
    SetUpMasterFiberEntity();
  }
}

void SchedulingImpl::Leave() noexcept {
  if (worker_index_ < group_size_) {
    vtm_[worker_index_] = 0;
  }
  worker_index_ = kUninitializedWorkerIndex;
}

void SchedulingImpl::Stop() noexcept {
  stopped_.store(true, std::memory_order_relaxed);
  notifier_->Notify(true);
}

// The following source codes are from taskflow.
// Copied and modified from
// https://github.com/taskflow/taskflow/blob/master/taskflow/core/executor.hpp

void SchedulingImpl::Schedule() noexcept {
  FiberEntity* fiber_entity = nullptr;
  for (;;) {
    if (fiber_entity) {
      if (!fiber_entity->is_fiber_reactor) {
        if (num_actives_.fetch_add(1) == 0 && num_thieves_ == 0) {
          notifier_->Notify(false);
        }

        while (fiber_entity) {
          SetFiberRunning(fiber_entity);
          fiber_entity->Resume();

          fiber_entity = GetOrInstantiateFiber(local_queues_[worker_index_].Pop());
        }

        --num_actives_;
      } else {
        SetFiberRunning(fiber_entity);
        fiber_entity->Resume();
      }
    }

    fiber_entity = WaitForFiber();
    if (fiber_entity == kSchedulingGroupShuttingDown) {
      break;
    }
  }
}

FiberEntity* SchedulingImpl::WaitForFiber() noexcept {
  FiberEntity* fiber_entity = nullptr;

  if (num_thieves_.fetch_add(1) <= group_size_ / 2) {
    fiber_entity = ExploreTask();
  }

  if (fiber_entity) {
    if (num_thieves_.fetch_sub(1) == 1) {
      notifier_->Notify(false);
    }
    return fiber_entity;
  }

  notifier_->PrepareWait(waiters_[worker_index_]);

  // if (global_queue_.Size() > 0) {
  if (global_queue_.UnsafeSize() > 0) {
    notifier_->CancelWait(waiters_[worker_index_]);

    fiber_entity = GetOrInstantiateFiber(global_queue_.Pop());
    if (fiber_entity) {
      if (num_thieves_.fetch_sub(1) == 1) {
        notifier_->Notify(false);
      }
      return fiber_entity;
    } else {
      vtm_[worker_index_] = worker_index_;
      --num_thieves_;
      return nullptr;
    }
  }

  if (stopped_.load(std::memory_order_relaxed)) {
    notifier_->CancelWait(waiters_[worker_index_]);
    notifier_->Notify(true);
    --num_thieves_;
    return kSchedulingGroupShuttingDown;
  }

  if (num_thieves_.fetch_sub(1) == 1) {
    if (num_actives_) {
      notifier_->CancelWait(waiters_[worker_index_]);
      return nullptr;
    }
    // check all queues again
    for (std::size_t i = 0; i < group_size_; i++) {
      auto& wsq = local_queues_[i];
      if (!wsq.Empty()) {
        vtm_[worker_index_] = i;
        notifier_->CancelWait(waiters_[worker_index_]);
        return nullptr;
      }
    }
  }

  fiber_entity = AcquireReactorFiber();
  if (fiber_entity != nullptr) {
    notifier_->CancelWait(waiters_[worker_index_]);
    return fiber_entity;
  }

  // Now I really need to relinguish my self to others
  notifier_->CommitWait(waiters_[worker_index_]);
  return nullptr;
}

FiberEntity* SchedulingImpl::ExploreTask() noexcept {
  std::size_t num_steals = 0;
  FiberEntity* fiber_entity = nullptr;

  for (;;) {
    if (worker_index_ == vtm_[worker_index_]) {
      fiber_entity = GetOrInstantiateFiber(local_queues_[worker_index_].Pop());
      if (!fiber_entity) {
        fiber_entity = GetOrInstantiateFiber(global_queue_.Pop());
      }
    } else {
      fiber_entity = GetOrInstantiateFiber(local_queues_[vtm_[worker_index_]].Steal());
    }

    if (fiber_entity) {
      break;
    }

    if (++num_steals > group_size_ / 2) {
      break;
    }

    vtm_[worker_index_] = (vtm_[worker_index_] + 1) % group_size_;
  }

  return fiber_entity;
}

// End of source codes that are from taskflow.

FiberEntity* SchedulingImpl::AcquireReactorFiber() noexcept {
  if (auto rc = GetOrInstantiateFiber(fiber_reactor_queue_.Pop())) {
    return rc;
  }

  return stopped_.load(std::memory_order_relaxed) ? kSchedulingGroupShuttingDown : nullptr;
}

FiberEntity* SchedulingImpl::GetOrInstantiateFiber(RunnableEntity* entity) noexcept {
  if (!entity) {
    return nullptr;
  }

  if (auto p = trpc::internal::dyn_cast<FiberDesc>(entity)) {
    return InstantiateFiberEntity(scheduling_group_, p);
  }

  TRPC_DCHECK(trpc::internal::isa<FiberEntity>(entity));
  return static_cast<FiberEntity*>(entity);
}

void SchedulingImpl::SetFiberRunning(FiberEntity* fiber_entity) noexcept {
  {
    std::scoped_lock _(fiber_entity->scheduler_lock);

    TRPC_CHECK(fiber_entity->state == FiberState::Ready);
    fiber_entity->state = FiberState::Running;
  }

  SchedulingVar::GetInstance()->ready_run_latency.Update(ReadTsc() - fiber_entity->last_ready_tsc);
}

void SchedulingImpl::StartFibers(FiberDesc** start, FiberDesc** end) noexcept {
  if (TRPC_UNLIKELY(start == end)) {
    return;
  }

  auto tsc = ReadTsc();
  for (auto iter = start; iter != end; ++iter) {
    (*iter)->last_ready_tsc = tsc;
  }

  bool is_fiber_reactor = (*start)->is_fiber_reactor;
  auto s1 = reinterpret_cast<RunnableEntity**>(start), s2 = reinterpret_cast<RunnableEntity**>(end);
  for (auto iter = s1; iter != s2; ++iter) {
    QueueRunnableEntity(*iter, is_fiber_reactor);
  }
}

bool SchedulingImpl::StartFiber(FiberDesc* fiber) noexcept {
  fiber->last_ready_tsc = ReadTsc();
  return QueueRunnableEntity(fiber, fiber->is_fiber_reactor);
}

bool SchedulingImpl::QueueRunnableEntity(RunnableEntity* entity, bool is_fiber_reactor) noexcept {
  TRPC_DCHECK(!stopped_.load(std::memory_order_relaxed), "The scheduling group has been stopped.");

  bool ret = true;
  if (!is_fiber_reactor) {
    if (scheduling_group_ == SchedulingGroup::Current() && worker_index_ < group_size_) {
      ret = PushToLocalQueue(entity);
    } else {
      ret = PushToGlobalQueue(global_queue_, entity);
      notifier_->Notify(false);
    }
  } else {
    ret = PushToGlobalQueue(fiber_reactor_queue_, entity);
    notifier_->Notify(false);
  }

  return ret;
}

// bool SchedulingImpl::PushToGlobalQueue(detail::v2::GlobalQueue& global_queue, RunnableEntity* fiber,
bool SchedulingImpl::PushToGlobalQueue(detail::v1::RunQueue& global_queue, RunnableEntity* fiber,
                                       bool wait) noexcept {
  if (TRPC_UNLIKELY(!global_queue.Push(fiber))) {
    auto since = ReadSteadyClock();
    while (!global_queue.Push(fiber)) {
      TRPC_FMT_INFO_EVERY_SECOND(
          "Global Run queue overflow: {}. Too many ready fibers to run. If you're still "
          "not overloaded, consider increasing `trpc_fiber_run_queue_size`.", GetFiberQueueSize());
      if (TRPC_UNLIKELY(ReadSteadyClock() - since > 5s)) {
        TRPC_FMT_INFO_EVERY_SECOND("Failed to push fiber into ready queue after retrying for 5s. Retry again.");
      }
      if (!wait) {
        return false;
      }
      std::this_thread::sleep_for(10us);
    }
  }

  return true;
}

bool SchedulingImpl::PushToLocalQueue(RunnableEntity* fiber) noexcept {
  if (!local_queues_[worker_index_].Push(fiber)) {
    bool ret = PushToGlobalQueue(global_queue_, fiber);
    if (ret)  {
      notifier_->Notify(false);
    }
    return ret;
  }
  return true;
}

FiberEntity* SchedulingImpl::GetFiberEntity() noexcept {
  TRPC_CHECK(scheduling_group_ == SchedulingGroup::Current(), "scheduling group are inconsistent");

  auto* fiber_entity = local_queues_[worker_index_].Pop();
  if (fiber_entity) {
    return GetOrInstantiateFiber(fiber_entity);
  }

  return nullptr;
}

void SchedulingImpl::Suspend(FiberEntity* self, std::unique_lock<Spinlock>&& scheduler_lock) noexcept {
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity(), "`self` must be pointer to caller's `FiberEntity`.");
  TRPC_CHECK(scheduler_lock.owns_lock(), "Scheduler lock must be held by caller prior to call to this method.");
  TRPC_CHECK(self->state == FiberState::Running,
             "`Suspend()` is only for running fiber's use. If you want to `ReadyFiber()` "
             "yourself and `Suspend()`, what you really need is `Yield()`.");
  self->state = FiberState::Waiting;

  // Note that we need to hold `scheduler_lock` until we finished context swap.
  // Otherwise if we're in ready queue, we can be resume again even before we
  // stopped running. This will be disastrous.
  //
  // Do NOT pass `scheduler_lock` ('s pointer)` to cb. Instead, play with raw
  // lock.
  //
  // The reason is that, `std::unique_lock<...>::unlock` itself is not an atomic
  // operation (although `Spinlock` is).
  //
  // What this means is that, after unlocking the scheduler lock, and the fiber
  // starts to run again, `std::unique_lock<...>::owns_lock` does not
  // necessarily be updated in time (before the fiber checks it), which can lead
  // to subtle bugs.
  if (auto rc = GetFiberEntity()) {
    TRPC_ASSERT(self != rc);
    {
      std::scoped_lock _(rc->scheduler_lock);

      TRPC_CHECK(rc->state == FiberState::Ready);
      rc->state = FiberState::Running;
    }

    SchedulingVar::GetInstance()->ready_run_latency.Update(ReadTsc() - rc->last_ready_tsc);

    rc->ResumeOn([self_lock = scheduler_lock.release()]() { self_lock->unlock(); });
  } else {
     auto master = GetMasterFiberEntity();
     master->ResumeOn([self_lock = scheduler_lock.release()]() { self_lock->unlock(); });
  }

  // When we're back, we should be in the same fiber.
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity());
}

void SchedulingImpl::Resume(FiberEntity* to) noexcept {
  Resume(to, std::unique_lock(to->scheduler_lock));
}

void SchedulingImpl::Resume(FiberEntity* self, FiberEntity* to) noexcept {
  Resume(to, std::unique_lock(to->scheduler_lock));
}

void SchedulingImpl::Resume(FiberEntity* fiber, std::unique_lock<Spinlock>&& scheduler_lock) noexcept {
  TRPC_DCHECK(!stopped_.load(std::memory_order_relaxed), "The scheduling group has been stopped.");
  TRPC_DCHECK_NE(fiber, GetMasterFiberEntity(), "Master fiber should not be added to run queue.");
  auto pre_state = fiber->state;

  fiber->state = FiberState::Ready;
  fiber->scheduling_group = scheduling_group_;
  fiber->last_ready_tsc = ReadTsc();

  if (scheduler_lock) {
    scheduler_lock.unlock();
  }

  if (!fiber->is_fiber_reactor) {
    if (pre_state == FiberState::Yield) {
      PushToGlobalQueue(global_queue_, fiber, true);
      notifier_->Notify(false);
      return;
    }

    // handle fiber
    if (scheduling_group_ == SchedulingGroup::Current() && worker_index_ < group_size_) {
      PushToLocalQueue(fiber);
    } else {
      PushToGlobalQueue(global_queue_, fiber, true);
      notifier_->Notify(false);
    }
  } else {
    // reactor fiber
    PushToGlobalQueue(fiber_reactor_queue_, fiber, true);
    notifier_->Notify(false);
  }
}

void SchedulingImpl::Yield(FiberEntity* self) noexcept {
  // In future we can directly yield to next ready fiber. This way we can
  // eliminate a context switch.
  auto master = GetMasterFiberEntity();

  // Master's state is not maintained in a coherency way..
  master->state = FiberState::Ready;
  self->state = FiberState::Yield;

  SwitchTo(self, master);
}

void SchedulingImpl::SwitchTo(FiberEntity* self, FiberEntity* to) noexcept {
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity());

  // We need scheduler lock here actually (at least to comfort TSan). But so
  // long as this check does not fiber, we're safe without the lock I think.
  TRPC_CHECK(to->state == FiberState::Ready, "Fiber `to` is not in ready state.");
  TRPC_CHECK_NE(self, to, "Switch to yourself results in U.B.");

  // Ensure neither `self->scheduler_lock` nor
  // `to->scheduler_lock` is currrently held (by someone else).

  // We delay queuing `self` to run queue until `to` starts to run.
  to->ResumeOn([this, self]() { Resume(self, std::unique_lock(self->scheduler_lock)); });

  // When we're back, we should be in the same fiber.
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity());
}

std::size_t SchedulingImpl::GetFiberQueueSize() noexcept {
  // std::size_t total_size = global_queue_.Size();
  std::size_t total_size = global_queue_.UnsafeSize();
  for (std::size_t i = 0; i < group_size_; ++i) {
    total_size += local_queues_[i].Size();
  }
  return total_size;
}

}  // namespace trpc::fiber::detail::v2
