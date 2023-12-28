//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/waitable.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"

#include <array>
#include <utility>
#include <vector>

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/util/delayed_init.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::fiber::detail {

namespace {

// Utility for waking up a fiber sleeping on a `Waitable` asynchronously.
class AsyncWaker {
 public:
  // Initialize an `AsyncWaker`.
  AsyncWaker(SchedulingGroup* sg, FiberEntity* self, WaitBlock* wb)
      : sg_(sg), self_(self), wb_(wb) {}

  // The destructor does some sanity checks.
  ~AsyncWaker() { TRPC_CHECK_EQ(timer_, std::uint64_t(0), "Have you called `Cleanup()`?"); }

  // Set a timer to awake `self` once `expires_at` is reached.
  void SetTimer(std::chrono::steady_clock::time_point expires_at) {
    wait_cb_ = MakeRefCounted<WaitCb>();
    wait_cb_->waiter = self_;

    // This callback wakes us up if we times out.
    auto timer_cb = [wait_cb = wait_cb_ /* ref-counted */, wb = wb_](auto) {
      std::scoped_lock lk(wait_cb->lock);
      if (!wait_cb->awake) {  // It's (possibly) timed out.
        // We're holding the lock, and `wait_cb->awake` has not been set yet, so
        // `Cleanup()` cannot possibly finished yet. Therefore, we can be sure
        // `wb` is still alive.
        if (wb->satisfied.exchange(true, std::memory_order_relaxed)) {
          // Someone else satisfied the wait earlier.
          return;
        }
        wait_cb->waiter->scheduling_group->Resume(wait_cb->waiter);
      }
    };

    // Set the timeout timer.
    timer_ = sg_->CreateTimer(expires_at, timer_cb);
    sg_->EnableTimer(timer_);
  }

  // Prevent the timer set by this class from waking up `self` again.
  void Cleanup() {
    // If `timer_cb` has returned, nothing special; if `timer_cb` has never
    // started, nothing special. But if `timer_cb` is running, we need to
    // prevent it from `StartFiber` us again (when we immediately sleep on
    // another unrelated thing.).
    sg_->RemoveTimer(std::exchange(timer_, 0));
    {
      // Here is the trick.
      //
      // We're running now, therefore our `WaitBlock::satisfied` has been set.
      // Our `timer_cb` will check the flag, and bail out without waking us
      // again.
      std::scoped_lock _(wait_cb_->lock);
      wait_cb_->awake = true;
    }  // `wait_cb_->awake` has been set, so other fields of us won't be touched
       // by `timer_cb`. we're safe to destruct from now on.
  }

 private:
  // Ref counted as it's used both by us and an asynchronous timer.
  struct WaitCb : RefCounted<WaitCb> {
    Spinlock lock;
    FiberEntity* waiter;
    bool awake = false;
  };

  SchedulingGroup* sg_;
  FiberEntity* self_;
  WaitBlock* wb_;
  RefPtr<WaitCb> wait_cb_;
  std::uint64_t timer_ = 0;
};

}  // namespace

// Implementation of `Waitable` goes below.

bool Waitable::AddWaiter(WaitBlock* waiter) {
  std::scoped_lock _(lock_);

  if (persistent_awakened_) {
    return false;
  }
  waiters_.push_back(waiter);
  return true;
}

bool Waitable::TryRemoveWaiter(WaitBlock* waiter) {
  std::scoped_lock _(lock_);
  return waiters_.erase(waiter);
}

WaitBlock* Waitable::WakeOne() {
  std::scoped_lock _(lock_);
  while (true) {
    auto waiter = waiters_.pop_front();
    if (!waiter) {
      return nullptr;
    }
    // Memory order is guaranteed by `lock_`.
    if (waiter->waiter && waiter->satisfied.exchange(true, std::memory_order_relaxed)) {
      // For fiber waiter, it's awakened by someone else.
      continue;
    }
    return waiter;
  }
}

void Waitable::SetPersistentAwakened(WaitBlockList& wbs) {
  std::scoped_lock _(lock_);
  persistent_awakened_ = true;

  while (auto ptr = waiters_.pop_front()) {
    // Same as `WakeOne`.
    // `satisfied` is only applied to fiber context.
    if ((ptr->waiter != nullptr) && ptr->satisfied.exchange(true, std::memory_order_relaxed)) {
      continue;
    }

    // 1. Push back all fiber context which are not satisfied.
    // 2. Push back all pthread context.
    wbs.push_back(ptr);
  }
}

void Waitable::ResetAwakened() {
  std::scoped_lock _(lock_);
  persistent_awakened_ = false;
}

WaitableTimer::WaitableTimer(std::chrono::steady_clock::time_point expires_at)
    : sg_(SchedulingGroup::Current()),
      // Does it sense if we use object pool here?
      impl_(MakeRefCounted<WaitableRefCounted>()) {
  // We must not set the timer before `impl_` is initialized.
  timer_id_ = sg_->CreateTimer(expires_at, [ref = impl_](auto) { OnTimerExpired(std::move(ref)); });
  sg_->EnableTimer(timer_id_);
}

WaitableTimer::~WaitableTimer() { sg_->RemoveTimer(timer_id_); }

void WaitableTimer::wait() {
  TRPC_DCHECK(IsFiberContextPresent());

  auto current = GetCurrentFiberEntity();
  // `WaitBlock.chain` is initialized by `DoublyLinkedListEntry` itself.
  WaitBlock wb = {.waiter = current};
  std::unique_lock lk(current->scheduler_lock);
  if (impl_->AddWaiter(&wb)) {
    current->scheduling_group->Suspend(current, std::move(lk));
    // We'll be awakened by `OnTimerExpired()`.
  } else {
    // The timer has fired before, return immediately then.
  }
}

void WaitableTimer::OnTimerExpired(RefPtr<WaitableRefCounted> ref) {
  WaitBlockList wbs;
  ref->SetPersistentAwakened(wbs);
  while (true) {
    auto* wb = wbs.pop_front();
    if (!wb) {
      return;
    }
    // This is fiber waiter.
    if (wb->waiter) {
      wb->waiter->scheduling_group->Resume(wb->waiter);
      continue;
    }

    // This is pthread waiter.
    wb->futex.Wake(1);
  }
}

// Implementation of `Mutex` goes below.

void Mutex::unlock() {
  bool is_post = is_post_;
  is_post_ = false;

  auto was = count_.fetch_sub(1, std::memory_order_release);
  if (was == 1) {
    // Lucky day, no one is waiting on the mutex.
    //
    // Nothing to do.
    return;
  }
  TRPC_CHECK_GT(was, std::uint32_t(1));

  // We need this lock so as to see a consistent state between `count_` and
  // `impl_` ('s internal wait queue).
  std::unique_lock splk(slow_path_lock_);
  auto* wb = impl_.WakeOne();
  TRPC_CHECK(wb);  // Otherwise `was` must be 1 (as there's no waiter).
  splk.unlock();


  // This is fiber waiter.
  if (wb->waiter) {
    if (!is_post) {
      if (wb->waiter->scheduling_group == SchedulingGroup::Current()) {
        auto current = GetCurrentFiberEntity();
        if (current != GetMasterFiberEntity()) {
          wb->waiter->scheduling_group->Resume(GetCurrentFiberEntity(), wb->waiter);
          return;
        }
      }
    }

    wb->waiter->scheduling_group->Resume(wb->waiter);
    return;
  }

  // This is pthread waiter.
  wb->futex.Wake(1);
}

void Mutex::LockSlowFromFiber() {
  // It's locked, take the slow path.
  std::unique_lock splk(slow_path_lock_);

  // Tell the owner that we're waiting for the lock.
  if (count_.fetch_add(1, std::memory_order_acquire) == 0) {
    // The owner released the lock before we incremented `count_`.
    //
    // We're still kind of lucky.
    return;
  }

  // Bad luck then. First we add us to the wait chain.
  auto current = GetCurrentFiberEntity();
  std::unique_lock slk(current->scheduler_lock);
  WaitBlock wb = {.waiter = current};
  TRPC_CHECK(impl_.AddWaiter(&wb));  // This can't fail as we never call `SetPersistentAwakened()`.

  // Now the slow path lock can be unlocked.
  //
  // Indeed it's possible that we're awakened even before we call `Suspend()`,
  // but this issue is already addressed by `scheduler_lock` (which we're
  // holding).
  splk.unlock();

  // Wait until we're woken by `unlock()`.
  //
  // Given that `scheduler_lock` is held by us, anyone else who concurrently
  // tries to wake us up is blocking on it until `Suspend()` has completed.
  // Hence no race here.
  current->scheduling_group->Suspend(current, std::move(slk));

  // Lock's owner has awakened us up, the lock is in our hand then.
  TRPC_DCHECK(!impl_.TryRemoveWaiter(&wb));
  return;
}

void Mutex::LockSlowFromPthread() {
  std::unique_lock splk(slow_path_lock_);

  if (count_.fetch_add(1, std::memory_order_acquire) == 0) {
    return;
  }

  // Set waiter to nullptr as we are in pthread context.
  WaitBlock wb = {.waiter = nullptr};
  TRPC_CHECK(impl_.AddWaiter(&wb));
  trpc::FutexNotifier::State st = wb.futex.GetState();

  splk.unlock();

  // No need to handle return value as we only wait for awakening.
  wb.futex.Wait(st, nullptr);
}

// Implementation of `ConditionVariable` goes below.

void ConditionVariable::wait(std::unique_lock<Mutex>& lock) {
  TRPC_DCHECK(lock.owns_lock());

  wait_until(lock, std::chrono::steady_clock::time_point::max());
}

bool ConditionVariable::wait_until(
    std::unique_lock<Mutex>& lock,
    std::chrono::steady_clock::time_point expires_at) {
  if (IsFiberContextPresent()) {
    return WaitUntilFromFiber(lock, expires_at);
  }

  return WaitUntilFromPthread(lock, expires_at);
}

bool ConditionVariable::WaitUntilFromFiber(std::unique_lock<Mutex>& lock,
                                           std::chrono::steady_clock::time_point expires_at) {
  auto current = GetCurrentFiberEntity();
  auto sg = current->scheduling_group;
  bool use_timeout = expires_at != std::chrono::steady_clock::time_point::max();
  DelayedInit<AsyncWaker> awaker;

  // Add us to the wait queue.
  std::unique_lock slk(current->scheduler_lock);
  WaitBlock wb = {.waiter = current};
  TRPC_CHECK(impl_.AddWaiter(&wb));
  if (use_timeout) {  // Set a timeout if needed.
    awaker.Init(sg, current, &wb);
    awaker->SetTimer(expires_at);
  }

  // Not reschedule fiber when executing the following `lock.unlock()` code
  lock.mutex()->SetPost();

  // Release user's lock.
  lock.unlock();

  // Block until being waken up by either `notify_xxx` or the timer.
  sg->Suspend(current, std::move(slk));  // `slk` is released by `Suspend()`.

  // Try remove us from the wait chain. This operation will fail if we're
  // awakened by `notify_xxx()`.
  auto timeout = impl_.TryRemoveWaiter(&wb);

  if (awaker) {
    // Stop the timer we've set.
    awaker->Cleanup();
  }

  // Grab the lock again and return.
  lock.lock();
  return !timeout;
}

bool ConditionVariable::WaitUntilFromPthread(std::unique_lock<Mutex>& lock,
                                             std::chrono::steady_clock::time_point expires_at) {
  // Set waiter to nullptr as we are in pthread context.
  WaitBlock wb = {.waiter = nullptr};
  TRPC_CHECK(impl_.AddWaiter(&wb));
  trpc::FutexNotifier::State st = wb.futex.GetState();
  lock.unlock();

  bool ret = wb.futex.Wait(st, &expires_at);
  impl_.TryRemoveWaiter(&wb);
  lock.lock();
  return ret;
}

void ConditionVariable::notify_one() noexcept {
  auto* wb = impl_.WakeOne();
  if (!wb) {
    return;
  }
  // This is fiber waiter.
  if (wb->waiter) {
    wb->waiter->scheduling_group->Resume(wb->waiter);
    return;
  }

  // This is pthread waiter.
  wb->futex.Wake(1);
}

void ConditionVariable::notify_all() noexcept {
  while (true) {
    auto* wb = impl_.WakeOne();
    if (!wb) {
      return;
    }

    // This is fiber waiter.
    if (wb->waiter) {
      wb->waiter->scheduling_group->Resume(wb->waiter);
      continue;
    }

    // This is pthread waiter.
    wb->futex.Wake(1);
  }
}

// Implementation of `ExitBarrier` goes below.

ExitBarrier::ExitBarrier() : count_(1) {}

std::unique_lock<Mutex> ExitBarrier::GrabLock() {
  return std::unique_lock(lock_);
}

void ExitBarrier::UnsafeCountDown(std::unique_lock<Mutex> lk) {
  TRPC_CHECK(lk.owns_lock() && lk.mutex() == &lock_);

  // tsan reports a data race if we unlock the lock before notifying the
  // waiters. Although I think it's a false positive, keep the lock before
  // notifying doesn't seem to hurt performance much.
  //
  // lk.unlock();
  TRPC_CHECK_GT(count_, std::size_t(0));
  if (!--count_) {
    cv_.notify_one();
  }
}

void ExitBarrier::Wait() {
  std::unique_lock lk(lock_);
  return cv_.wait(lk, [this] { return count_ == 0; });
}

// Implementation of `Event` goes below.

void Event::Wait() {
  if (IsFiberContextPresent()) {
    WaitFromFiber();
    return;
  }

  WaitFromPthread();
}

void Event::WaitFromFiber() {
  auto current = GetCurrentFiberEntity();
  WaitBlock wb = {.waiter = current};
  std::unique_lock lk(current->scheduler_lock);
  if (impl_.AddWaiter(&wb)) {
    current->scheduling_group->Suspend(current, std::move(lk));
  } else {
    // The event is set already, return immediately.
  }
}

void Event::WaitFromPthread() {
  // Set waiter to nullptr as we are in pthread context.
  WaitBlock wb = {.waiter = nullptr};
  // Must get state before AddWaiter.
  trpc::FutexNotifier::State st = wb.futex.GetState();
  if (impl_.AddWaiter(&wb)) {
    wb.futex.Wait(st, nullptr);
  } else {
    // The event is set already, return immediately.
  }
}

void Event::Set() {
  WaitBlockList wbs;
  impl_.SetPersistentAwakened(wbs);

  // Fiber wake-up must be delayed until we're done with `impl_`, otherwise
  // `impl_` can be destroyed after its emptied but before we touch it again.
  while (true) {
    auto* wb = wbs.pop_front();
    if (!wb) {
      return;
    }

    // This is fiber waiter.
    if (wb->waiter) {
      wb->waiter->scheduling_group->Resume(wb->waiter);
      continue;
    }

    // This is pthread waiter.
    wb->futex.Wake(1);
  }
}

OneshotTimedEvent::OneshotTimedEvent(std::chrono::steady_clock::time_point expires_at)
    : sg_(SchedulingGroup::Current()), impl_(MakeRefCounted<Impl>()) {
  timer_id_ = sg_->CreateTimer(expires_at, [ref = impl_](auto) { OnTimerExpired(std::move(ref)); });
  sg_->EnableTimer(timer_id_);
}

OneshotTimedEvent::~OneshotTimedEvent() { sg_->RemoveTimer(timer_id_); }

void OneshotTimedEvent::Wait() { impl_->event.Wait(); }

void OneshotTimedEvent::Set() { impl_->IdempotentSet(); }

void OneshotTimedEvent::Impl::IdempotentSet() {
  if (!event_set_guard.exchange(true, std::memory_order_relaxed)) {
    event.Set();
  }
}

void OneshotTimedEvent::OnTimerExpired(RefPtr<Impl> ref) { ref->IdempotentSet(); }

}  // namespace trpc::fiber::detail
