//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/scheduling_group.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v1/scheduling_impl.h"

#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>

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

namespace trpc::fiber::detail::v1 {

using namespace std::literals;

void SchedulingImpl::WaitSlot::Wake() noexcept {
  if (wakeup_count_.fetch_add(1, std::memory_order_relaxed) == 0) {
    TRPC_PCHECK(syscall(SYS_futex, &wakeup_count_, FUTEX_WAKE_PRIVATE, 1, 0, 0, 0) >= 0);
  }
  // If `Wait()` is called before this check fires, `wakeup_count_` can be 0.
  TRPC_CHECK_GE(wakeup_count_.load(std::memory_order_relaxed), 0);
}

void SchedulingImpl::WaitSlot::Wait() noexcept {
  if (wakeup_count_.fetch_sub(1, std::memory_order_relaxed) == 1) {
    do {
      auto rc = syscall(SYS_futex, &wakeup_count_, FUTEX_WAIT_PRIVATE, 0, 0, 0, 0);
      TRPC_PCHECK(rc == 0 || errno == EAGAIN);
    } while (wakeup_count_.load(std::memory_order_relaxed) == 0);
  }
  TRPC_CHECK_GT(wakeup_count_.load(std::memory_order_relaxed), 0);
}

void SchedulingImpl::WaitSlot::PersistentWake() noexcept {
  // Hopefully this is large enough.
  wakeup_count_.store(0x4000'0000, std::memory_order_relaxed);
  TRPC_PCHECK(syscall(SYS_futex, &wakeup_count_, FUTEX_WAKE_PRIVATE, INT_MAX, 0, 0, 0) >= 0);
}

FiberEntity* const kSchedulingGroupShuttingDown = reinterpret_cast<FiberEntity*>(0x1);
thread_local std::size_t SchedulingImpl::worker_index_ = kUninitializedWorkerIndex;

bool SchedulingImpl::Init(SchedulingGroup* scheduling_group,
                          std::size_t scheduling_group_size) noexcept {
  scheduling_group_ = scheduling_group;
  group_size_ = scheduling_group_size;

  TRPC_ASSERT(run_queue_.Init(GetFiberRunQueueSize()));

  wait_slots_ = std::make_unique<WaitSlot[]>(group_size_);

  steal_vec_clock_ = std::make_unique<std::uint64_t[]>(group_size_);
  for (size_t i = 0; i < group_size_; ++i) {
    steal_vec_clock_[i] = 0;
  }

  victims_ = std::make_unique<std::priority_queue<Victim>[]>(group_size_);

  return true;
}

void SchedulingImpl::AddForeignSchedulingGroup(std::size_t worker_index, SchedulingGroup* sg,
                                               std::uint64_t steal_every_n) noexcept {
  TRPC_CHECK_LT(worker_index, group_size_);
  victims_[worker_index].push({.sg = sg, .steal_every_n = steal_every_n, .next_steal = trpc::Random(steal_every_n)});
}

FiberEntity* SchedulingImpl::StealFiberFromForeignSchedulingGroup() noexcept {
  TRPC_CHECK_LT(worker_index_, group_size_);
  if (victims_[worker_index_].empty()) {
    return nullptr;
  }

  ++steal_vec_clock_[worker_index_];
  while (victims_[worker_index_].top().next_steal <= steal_vec_clock_[worker_index_]) {
    auto&& top = victims_[worker_index_].top();
    if (auto rc = top.sg->RemoteAcquireFiber()) {
      // We don't pop the top in this case, since it's not empty, maybe the next
      // time we try to steal, there are still something for us.
      return rc;
    }
    victims_[worker_index_].push({.sg = top.sg,
                   .steal_every_n = top.steal_every_n,
                   .next_steal = top.next_steal + top.steal_every_n});
    victims_[worker_index_].pop();
  }

  return nullptr;
}

void SchedulingImpl::Enter(std::size_t index) noexcept {
  worker_index_ = index;

  // Initialize master fiber for this worker.
  if (worker_index_ != SchedulingGroup::kTimerWorkerIndex) {
    SetUpMasterFiberEntity();
  }
}

void SchedulingImpl::Schedule() noexcept {
  while (true) {
    auto fiber = AcquireFiber();

    if (!fiber) {
      fiber = SpinningAcquireFiber();
      if (!fiber) {
        fiber = StealFiberFromForeignSchedulingGroup();
        TRPC_CHECK_NE(fiber, kSchedulingGroupShuttingDown);
        if (!fiber) {
          fiber = WaitForFiber();
          TRPC_CHECK_NE(fiber, static_cast<trpc::fiber::detail::FiberEntity*>(nullptr));
        }
      }
    }

    if (TRPC_UNLIKELY(fiber == kSchedulingGroupShuttingDown)) {
      break;
    }

    fiber->Resume();

    // HeartBeat(run_queue_.UnsafeSize());
  }
}

void SchedulingImpl::Leave() noexcept {
  worker_index_ = kUninitializedWorkerIndex;
}

FiberEntity* SchedulingImpl::AcquireFiber() noexcept {
  if (auto rc = GetOrInstantiateFiber(run_queue_.Pop())) {
    {
      // Acquiring the lock here guarantees us anyone who is working on this fiber
      // (with the lock held) has done its job before we returning it to the
      // caller (worker).
      std::scoped_lock _(rc->scheduler_lock);

      TRPC_CHECK(rc->state == FiberState::Ready);
      rc->state = FiberState::Running;
    }

    SchedulingVar::GetInstance()->ready_run_latency.Update(ReadTsc() - rc->last_ready_tsc);

    return rc;
  }

  return stopped_.load(std::memory_order_relaxed) ? kSchedulingGroupShuttingDown : nullptr;
}

FiberEntity* SchedulingImpl::SpinningAcquireFiber() noexcept {
  // We don't want too many workers spinning, it wastes CPU cycles.
  static constexpr auto kMaximumSpinners = 2;

  TRPC_CHECK_NE(worker_index_, kUninitializedWorkerIndex);
  TRPC_CHECK_LT(worker_index_, group_size_);

  FiberEntity* fiber = nullptr;
  auto spinning = spinning_workers_.load(std::memory_order_relaxed);
  auto mask = 1ULL << worker_index_;
  bool need_spin = false;

  // Simply test `spinning` and try to spin may result in too many workers to
  // spin, as it there's a time windows between we test `spinning` and set our
  // bit in it.
  while (CountNonZeros(spinning) < kMaximumSpinners) {
    TRPC_DCHECK_EQ(spinning & mask, std::uint64_t(0));
    if (spinning_workers_.compare_exchange_weak(spinning, spinning | mask,
                                                std::memory_order_relaxed)) {
      need_spin = true;
      break;
    }
  }

  if (need_spin) {
    static constexpr auto kMaximumCyclesToSpin = 10'000;
    // Wait for some time between touching `run_queue_` to reduce contention.
    static constexpr auto kCyclesBetweenRetry = 1000;
    auto start = ReadTsc(), end = start + kMaximumCyclesToSpin;

    ScopedDeferred _([&] {
      // Note that we can actually clear nothing, the same bit can be cleared by
      // `WakeOneSpinningWorker` simultaneously. This is okay though, as we'll
      // try `AcquireFiber()` when we leave anyway.
      spinning_workers_.fetch_and(~mask, std::memory_order_relaxed);
    });

    do {
      if (auto rc = AcquireFiber()) {
        fiber = rc;
        break;
      }
      auto next = start + kCyclesBetweenRetry;
      while (start < next) {
        if (pending_spinner_wakeup_.load(std::memory_order_relaxed) &&
            pending_spinner_wakeup_.exchange(false, std::memory_order_relaxed)) {
          // There's a pending wakeup, and it's us who is chosen to finish this
          // job.
          WakeUpOneDeepSleepingWorker();
        } else {
          Pause<16>();
        }
        start = ReadTsc();
      }
    } while (start < end && (spinning_workers_.load(std::memory_order_relaxed) & mask));
  } else {
    // Otherwise there are already at least 2 workers spinning, don't waste CPU
    // cycles then.
    return nullptr;
  }

  if (fiber || ((fiber = AcquireFiber()))) {
    // Given that we successfully grabbed a fiber to run, we're likely under
    // load. So wake another worker to spin (if there are not enough spinners).
    //
    // We don't want to wake it here, though, as we've had something useful to
    // do (run `fiber)`, so we leave it for other spinners to wake as they have
    // nothing useful to do anyway.
    if (CountNonZeros(spinning_workers_.load(std::memory_order_relaxed)) < kMaximumSpinners) {
      pending_spinner_wakeup_.store(true, std::memory_order_relaxed);
    }
  }
  return fiber;
}

FiberEntity* SchedulingImpl::WaitForFiber() noexcept {
  TRPC_CHECK_NE(worker_index_, kUninitializedWorkerIndex);
  TRPC_CHECK_LT(worker_index_, group_size_);
  auto mask = 1ULL << worker_index_;

  while (true) {
    ScopedDeferred _([&] {
      // If we're woken up before we even sleep (i.e., we're "woken up" after we
      // set the bit in `sleeping_workers_` but before we actually called
      // `WaitSlot::Wait()`), this effectively clears nothing.
      sleeping_workers_.fetch_and(~mask, std::memory_order_relaxed);
    });
    TRPC_CHECK_EQ(sleeping_workers_.fetch_or(mask, std::memory_order_relaxed) & mask,
                  std::uint64_t(0));

    // We should test if the queue is indeed empty, otherwise if a new fiber is
    // put into the ready queue concurrently, and whoever makes the fiber ready
    // checked the sleeping mask before we updated it, we'll lose the fiber.
    if (auto f = AcquireFiber()) {
      // A new fiber is put into ready queue concurrently then.
      //
      // If our sleeping mask has already been cleared (by someone else), we
      // need to wake up yet another sleeping worker (otherwise it's a wakeup
      // miss.).
      //
      // Note that in this case the "deferred" clean up is not needed actually.
      // This is a rare case, though.
      if ((sleeping_workers_.fetch_and(~mask, std::memory_order_relaxed) & mask) == 0) {
        // Someone woken us up before we cleared the flag, wake up a new worker
        // for him.
        WakeUpOneWorker();
      }
      return f;
    }

    wait_slots_[worker_index_].Wait();

    // We only return non-`nullptr` here. If we return `nullptr` to the caller,
    // it'll go spinning immediately. Doing that will likely waste CPU cycles.
    if (auto f = AcquireFiber()) {
      return f;
    }  // Otherwise try again (and possibly sleep) until a fiber is ready.
  }
}

FiberEntity* SchedulingImpl::RemoteAcquireFiber() noexcept {
  if (auto rc = GetOrInstantiateFiber(run_queue_.Steal())) {
    std::scoped_lock _(rc->scheduler_lock);

    TRPC_CHECK(rc->state == FiberState::Ready);
    rc->state = FiberState::Running;

    // SchedulingVar::GetInstance()->ready_run_latency.Update(ReadTsc() - rc->last_ready_tsc);

    // It now belongs to the caller's scheduling group.
    rc->scheduling_group = SchedulingGroup::Current();
    return rc;
  }
  return nullptr;
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

bool SchedulingImpl::StartFiber(FiberDesc* desc) noexcept {
  desc->last_ready_tsc = ReadTsc();
  return QueueRunnableEntity(desc, desc->scheduling_group_local);
}

void SchedulingImpl::StartFibers(FiberDesc** start, FiberDesc** end) noexcept {
  if (TRPC_UNLIKELY(start == end)) {
    return;
  }

  auto tsc = ReadTsc();
  for (auto iter = start; iter != end; ++iter) {
    (*iter)->last_ready_tsc = tsc;
  }

  auto s1 = reinterpret_cast<RunnableEntity**>(start),
       s2 = reinterpret_cast<RunnableEntity**>(end);
  if (TRPC_UNLIKELY(!run_queue_.BatchPush(s1, s2, false))) {
    auto since = ReadSteadyClock();

    while (!run_queue_.BatchPush(s1, s2, false)) {
      TRPC_FMT_INFO_EVERY_SECOND(
          "Run queue overflow. Too many ready fibers to run. If you're still "
          "not overloaded, consider increasing `trpc_fiber_run_queue_size`.");
      if (TRPC_UNLIKELY(ReadSteadyClock() - since > 2s)) {
        TRPC_FMT_INFO_EVERY_SECOND(
            "Failed to push fiber into ready queue after retrying for 2s. Retry again.");
      }
      std::this_thread::sleep_for(100us);
    }
  }

  WakeUpWorkers(end - start);
}

bool SchedulingImpl::QueueRunnableEntity(RunnableEntity* entity,
                                         bool sg_local, bool wait) noexcept {
  TRPC_DCHECK(!stopped_.load(std::memory_order_relaxed), "The scheduling group has been stopped.");

  // Push the fiber into run queue and (optionally) wake up a worker.
  if (TRPC_UNLIKELY(!run_queue_.Push(entity, sg_local))) {
    auto since = ReadSteadyClock();

    while (!run_queue_.Push(entity, sg_local)) {
      TRPC_FMT_INFO_EVERY_SECOND(
          "Run queue overflow. Too many ready fibers to run. If you're still "
          "not overloaded, consider increasing `trpc_fiber_run_queue_size`.");
      if (TRPC_UNLIKELY(ReadSteadyClock() - since > 2s)) {
        TRPC_FMT_INFO_EVERY_SECOND(
            "Failed to push fiber into ready queue after retrying for 2s. Retry again.");
        if (!wait) {
          return false;
        }
      }
      std::this_thread::sleep_for(100us);
    }
  }

  WakeUpOneWorker();

  return true;
}

void SchedulingImpl::Suspend(FiberEntity* self, std::unique_lock<Spinlock>&& scheduler_lock) noexcept {
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity(), "`self` must be pointer to caller's `FiberEntity`.");
  TRPC_CHECK(scheduler_lock.owns_lock(),
             "Scheduler lock must be held by caller prior to call to this method.");
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
  if (auto rc = GetOrInstantiateFiber(run_queue_.Pop())) {
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
  PostResume(to);
}

void SchedulingImpl::Resume(FiberEntity* self, FiberEntity* to) noexcept {
  TRPC_DCHECK_NE(self, GetMasterFiberEntity(), "Current fiber not be master fiber.");
  TRPC_DCHECK_NE(to, GetMasterFiberEntity(), "Master fiber should not be added to run queue.");
  TRPC_ASSERT(self != to);

  {
    std::unique_lock lk(to->scheduler_lock);
    to->state = FiberState::Running;
    to->scheduling_group = scheduling_group_;
  }

  to->ResumeOn([this, self]() {
    PostResume(self);
  });

  // When we're back, we should be in the same fiber.
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity());
}

void SchedulingImpl::PostResume(FiberEntity* fiber) noexcept {
  TRPC_DCHECK_NE(fiber, GetMasterFiberEntity(), "Master fiber should not be added to run queue.");

  {
    std::unique_lock lk(fiber->scheduler_lock);

    fiber->state = FiberState::Ready;
    fiber->scheduling_group = scheduling_group_;
    fiber->last_ready_tsc = ReadTsc();
  }

  QueueRunnableEntity(fiber, fiber->scheduling_group_local, true);
}

void SchedulingImpl::Yield(FiberEntity* self) noexcept {
  if (auto rc = GetOrInstantiateFiber(run_queue_.Pop())) {
    {
      std::scoped_lock _(rc->scheduler_lock);

      TRPC_CHECK(rc->state == FiberState::Ready);
      rc->state = FiberState::Running;
    }

    SchedulingVar::GetInstance()->ready_run_latency.Update(ReadTsc() - rc->last_ready_tsc);

    rc->ResumeOn([this, self]() {
      PostResume(self);
    });

    // When we're back, we should be in the same fiber.
    TRPC_CHECK_EQ(self, GetCurrentFiberEntity());
  } else {
    auto master = GetMasterFiberEntity();

    // Master's state is not maintained in a coherency way..
    master->state = FiberState::Ready;
    SwitchTo(self, master);
  }
}

void SchedulingImpl::SwitchTo(FiberEntity* self, FiberEntity* to) noexcept {
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity());

  // We need scheduler lock here actually (at least to comfort TSan). But so
  // long as this check does not fiber, we're safe without the lock I think.
  TRPC_CHECK(to->state == FiberState::Ready, "Fiber `to` is not in ready state.");
  TRPC_CHECK_NE(self, to, "Switch to yourself results in U.B.");

  // We delay queuing `self` to run queue until `to` starts to run.
  //
  // It's possible that we first add `self` to run queue with its scheduler lock
  // locked, and unlock the lock when `to` runs. However, if `self` is grabbed
  // by some worker prior `to` starts to run, the worker will spin to wait for
  // `to`. This can be quite costly.
  // to->ResumeOn([this, self]() { ReadyFiber(self, std::unique_lock(self->scheduler_lock)); });
  to->ResumeOn([this, self]() {
    PostResume(self);
  });

  // When we're back, we should be in the same fiber.
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity());
}

void SchedulingImpl::Stop() noexcept {
  stopped_.store(true, std::memory_order_relaxed);
  for (std::size_t index = 0; index != group_size_; ++index) {
    wait_slots_[index].PersistentWake();
  }
}

bool SchedulingImpl::WakeUpOneWorker() noexcept {
  return WakeUpOneSpinningWorker() || WakeUpOneDeepSleepingWorker();
}

bool SchedulingImpl::WakeUpOneSpinningWorker() noexcept {
  // FIXME: Is "relaxed" order sufficient here?
  while (auto spinning_mask = spinning_workers_.load(std::memory_order_relaxed)) {
    // Last fiber worker (LSB in `spinning_mask`) that is spinning.
    auto last_spinning = __builtin_ffsll(spinning_mask) - 1;
    auto claiming_mask = 1ULL << last_spinning;
    if (TRPC_LIKELY(spinning_workers_.fetch_and(~claiming_mask, std::memory_order_relaxed) &
                    claiming_mask)) {
      // We cleared the `last_spinning` bit, no one else will try to dispatch
      // work to it.
      // SchedulingVar::GetInstance()->spinning_worker_wakeups.Add(1);
      return true;  // Fast path then.
    }
    Pause();
  }  // Keep trying until no one else is spinning.
  return false;
}

bool SchedulingImpl::WakeUpWorkers(std::size_t n) noexcept {
  if (TRPC_UNLIKELY(n == 0)) {
    return false;  // No worker is waken up.
  }
  if (TRPC_UNLIKELY(n == 1)) {
    return WakeUpOneWorker();
  }

  // As there are at most two spinners, and `n` is at least two, we can safely
  // claim all spinning workers.
  auto spinning_mask_was = spinning_workers_.exchange(0, std::memory_order_relaxed);
  auto woke = CountNonZeros(spinning_mask_was);
  TRPC_CHECK_LE(static_cast<std::size_t>(woke), n);
  n -= woke;

  if (n >= group_size_) {
    // If there are more fibers than the group size, wake up all workers.
    auto sleeping_mask_was = sleeping_workers_.exchange(0, std::memory_order_relaxed);
    for (std::size_t i = 0; i != group_size_; ++i) {
      if (sleeping_mask_was & (1 << i)) {
        wait_slots_[i].Wake();
      }
    }
    return true;
  } else if (n >= 1) {
    while (auto sleeping_mask_was = sleeping_workers_.load(std::memory_order_relaxed)) {
      auto mask_to = sleeping_mask_was;
      // Wake up workers with lowest indices.
      if (static_cast<std::size_t>(CountNonZeros(sleeping_mask_was)) <= n) {
        mask_to = 0;  // All workers will be woken up.
      } else {
        while (n--) {
          mask_to &= ~(1 << __builtin_ffsll(mask_to));
        }
      }

      // Try to claim the workers.
      if (TRPC_LIKELY(sleeping_workers_.compare_exchange_weak(sleeping_mask_was, mask_to,
                                                              std::memory_order_relaxed))) {
        auto masked = sleeping_mask_was & ~mask_to;
        for (std::size_t i = 0; i != group_size_; ++i) {
          if (masked & (1 << i)) {
            wait_slots_[i].Wake();
          }
        }
        return true;
      }
      Pause();
    }
  } else {
    return true;
  }
  return false;
}

bool SchedulingImpl::WakeUpOneDeepSleepingWorker() noexcept {
  // We indeed have to wake someone that is in deep sleep then.
  while (auto sleeping_mask = sleeping_workers_.load(std::memory_order_relaxed)) {
    // We always give a preference to workers with a lower index (LSB in
    // `sleeping_mask`).
    //
    // If we're under light load, this way we hopefully can avoid wake workers
    // with higher index at all.
    auto last_sleeping = __builtin_ffsll(sleeping_mask) - 1;
    auto claiming_mask = 1ULL << last_sleeping;
    if (TRPC_LIKELY(sleeping_workers_.fetch_and(~claiming_mask, std::memory_order_relaxed) &
                    claiming_mask)) {
      // We claimed the worker. Let's wake it up then.
      //
      // `WaitSlot` class it self guaranteed no wake-up loss. So don't worry
      // about that.
      TRPC_CHECK_LT(static_cast<std::size_t>(last_sleeping), group_size_);
      wait_slots_[last_sleeping].Wake();
      return true;
    }
    Pause();
  }
  return false;
}

std::size_t SchedulingImpl::GetFiberQueueSize() noexcept {
  return run_queue_.UnsafeSize();
}

}  // namespace trpc::fiber::detail::v1
