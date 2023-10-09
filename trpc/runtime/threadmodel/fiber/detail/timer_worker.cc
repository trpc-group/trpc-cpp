//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/timer_worker.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/thread/spinlock.h"
#include "trpc/util/thread/thread_helper.h"

using namespace std::literals;

namespace trpc {

namespace object_pool {

template <>
struct ObjectPoolTraits<fiber::detail::TimerWorker::Entry> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

}  // namespace trpc

namespace trpc::fiber::detail {

namespace {

// Load time point from `expected`, but if it's infinite, return a large timeout
// instead (otherwise overflow can occur in libstdc++, which results in no wait
// at all.).
std::chrono::steady_clock::time_point GetSleepTimeout(
    std::chrono::steady_clock::duration expected) {
  if (expected == std::chrono::steady_clock::duration::max()) {
    return ReadSteadyClock() + 10000s;  // Randomly chosen.
  } else {
    return std::chrono::steady_clock::time_point(expected);
  }
}

}  // namespace

struct TimerWorker::Entry : object_pool::EnableLwSharedFromThis<Entry> {
  Entry() {chain.next = chain.prev = &chain;}
  DoublyLinkedListEntry chain;
  Spinlock lock;  // Protects `cb`.
  std::atomic<bool> cancelled{false};
  bool periodic{false};
  TimerWorker* owner;
  Function<void(std::uint64_t)> cb;
  std::chrono::steady_clock::time_point expires_at;
  std::chrono::nanoseconds interval;
};

struct TimerWorker::ThreadLocalQueue {
  using EntryList = DoublyLinkedList<Entry, &Entry::chain>;
  Spinlock lock;  // To be clear, our critical section size indeed isn't stable
                  // (as we can incur heap memory allocation inside it).
                  // However, we don't expect the lock to contend much, using a
                  // `std::mutex` (which incurs a function call to
                  // `pthread_mutex_lock`) here is too high a price to pay.
  EntryList timers;
  std::chrono::steady_clock::time_point earliest =
      std::chrono::steady_clock::time_point::max();

  // This seemingly useless destructor comforts TSan. Otherwise a data race will
  // be reported between this queue's destruction and its last read (by
  // `TimerWorker`).
  //
  // Admittedly it's a race, but it only happens when worker exits (i.e. program
  // exits), so we don't care about it.
  ~ThreadLocalQueue() { std::scoped_lock _(lock); }
};

thread_local bool tls_queue_initialized = false;

TimerWorker::TimerWorker(SchedulingGroup* sg, bool disable_process_name)
    // `+ 1` below for our own worker thread.
    : sg_(sg),
      disable_process_name_(disable_process_name),
      latch(sg_->GroupSize() + 1),
      producers_(sg_->GroupSize() + 1) {}

TimerWorker::~TimerWorker() = default;

TimerWorker* TimerWorker::GetTimerOwner(std::uint64_t timer_id) {
  return reinterpret_cast<Entry*>(timer_id)->owner;
}

std::uint64_t TimerWorker::CreateTimer(
    std::chrono::steady_clock::time_point expires_at,
    Function<void(std::uint64_t)>&& cb) {
  TRPC_CHECK(cb, "No callback for the timer?");

  auto ptr = object_pool::MakeLwShared<Entry>();
  ptr->owner = this;
  ptr->cancelled.store(false, std::memory_order_relaxed);
  ptr->cb = std::move(cb);
  ptr->expires_at = expires_at;
  ptr->periodic = false;

  TRPC_DCHECK_EQ(ptr->UseCount(), std::uint32_t(1));
  return reinterpret_cast<std::uint64_t>(ptr.Leak());
}

std::uint64_t TimerWorker::CreateTimer(
    std::chrono::steady_clock::time_point initial_expires_at,
    std::chrono::nanoseconds interval, Function<void(std::uint64_t)>&& cb) {
  TRPC_CHECK(cb, "No callback for the timer?");
  TRPC_CHECK(interval > 0ns,
              "`interval` must be greater than 0 for periodic timers.");

  auto ptr = object_pool::MakeLwShared<Entry>();
  ptr->owner = this;
  ptr->cancelled.store(false, std::memory_order_relaxed);
  ptr->cb = std::move(cb);
  ptr->expires_at = initial_expires_at;
  ptr->interval = interval;
  ptr->periodic = true;

  TRPC_DCHECK_EQ(ptr->UseCount(), std::uint32_t(1));
  return reinterpret_cast<std::uint64_t>(ptr.Leak());
}

void TimerWorker::EnableTimer(std::uint64_t timer_id) {
  // Ref-count is incremented here. We'll be holding the timer internally.
  AddTimer(
      object_pool::LwSharedPtr(object_pool::lw_shared_ref_ptr, reinterpret_cast<Entry*>(timer_id)));
}

void TimerWorker::RemoveTimer(std::uint64_t timer_id) {
  EntryPtr ptr(object_pool::lw_shared_adopt_ptr, reinterpret_cast<Entry*>(timer_id));
  TRPC_CHECK_EQ(ptr->owner, this,
                 "The timer you're trying to detach does not belong to this "
                 "scheduling group.");
  Function<void(std::uint64_t)> cb;
  {
    std::scoped_lock _(ptr->lock);
    ptr->cancelled.store(true, std::memory_order_relaxed);
    cb = std::move(ptr->cb);
  }
  // `cb` is released out of the (timer's) lock.
  // Ref-count on `timer_id` is implicitly release by destruction of `ptr`.
}

void TimerWorker::DetachTimer(std::uint64_t timer_id) {
  EntryPtr timer(object_pool::lw_shared_adopt_ptr, reinterpret_cast<Entry*>(timer_id));
  TRPC_CHECK_EQ(timer->owner, this,
                 "The timer you're trying to detach does not belong to this "
                 "scheduling group.");
  // Ref-count on `timer` is released.
}

SchedulingGroup* TimerWorker::GetSchedulingGroup() { return sg_; }

void TimerWorker::InitializeLocalQueue(std::size_t worker_index) {
  if (worker_index == SchedulingGroup::kTimerWorkerIndex) {
    worker_index = sg_->GroupSize();
  }
  TRPC_CHECK_LT(worker_index, sg_->GroupSize() + 1);
  TRPC_CHECK(producers_[worker_index] == nullptr,
              "Someone else has registered itself as worker #{}.",
              worker_index);
  producers_[worker_index] = GetThreadLocalQueue();
  tls_queue_initialized = true;
  latch.count_down();
}

void TimerWorker::Start() {
  worker_ = std::thread([&] {
    if (!sg_->Affinity().empty()) {
      trpc::SetCurrentThreadAffinity(sg_->Affinity());
    }
    if (disable_process_name_) {
      trpc::SetCurrentThreadName("TimerWorker");
    }
    WorkerProc();
  });
}

void TimerWorker::Stop() {
  std::scoped_lock _(lock_);
  stopped_.store(true, std::memory_order_relaxed);
  cv_.notify_one();
}

void TimerWorker::Join() { worker_.join(); }

void TimerWorker::WorkerProc() {
  sg_->EnterGroup(SchedulingGroup::kTimerWorkerIndex);
  WaitForWorkers();  // Wait for other workers to come in.

  while (!stopped_.load(std::memory_order_relaxed)) {
    // We need to reset `next_expires_at_` to a large value, otherwise if
    // someone is inserting a timer that fires later than `next_expires_at_`, it
    // won't touch `next_expires_at_` to reflect this. Later one when we reset
    // `next_expires_at_` (in this method, by calling `WakeWorkerIfNeeded`),
    // that timer will be delayed.
    //
    // This can cause some unnecessary wake up of `cv_` (by
    // `WakeWorkerIfNeeded`), but the wake up operation should be infrequently
    // anyway.
    next_expires_at_.store(std::chrono::steady_clock::duration::max(),
                           std::memory_order_relaxed);

    // Collect thread-local timer queues into our central heap.
    ReapThreadLocalQueues();

    // And fire those who has expired.
    FireTimers();

    if (!timers_.empty()) {
      // Do not reset `next_expires_at_` directly here, we need to compare our
      // earliest timer with thread-local queues (which is handled by this
      // `WakeWorkerIfNeeded`).
      WakeWorkerIfNeeded(timers_.top()->expires_at);
    }

    // Sleep until next time fires.
    std::unique_lock lk(lock_);
    auto expected = next_expires_at_.load(std::memory_order_relaxed);
    cv_.wait_until(lk, GetSleepTimeout(expected), [&] {
      // We need to check `next_expires_at_` to see if it still equals to the
      // time we're expected to be awakened. If it doesn't, we need to wake up
      // early since someone else must added an earlier timer (and, as a
      // consequence, changed `next_expires_at_`.).
      return next_expires_at_.load(std::memory_order_relaxed) != expected ||
             stopped_.load(std::memory_order_relaxed);
    });
  }
  sg_->LeaveGroup();
}

void TimerWorker::AddTimer(EntryPtr timer) {
  TRPC_CHECK(tls_queue_initialized,
              "You must initialize your thread-local queue (done as part of "
              "`SchedulingGroup::EnterGroup()` before calling `AddTimer`.");
  TRPC_DCHECK_EQ(timer->UseCount(), std::uint32_t(2));  // One is caller, one is us.

  auto&& tls_queue = GetThreadLocalQueue();
  std::unique_lock lk(tls_queue->lock);  // This is cheap (relatively, I mean).
  auto expires_at = timer->expires_at;
  tls_queue->timers.push_back(timer.Leak());

  if (tls_queue->earliest > expires_at) {
    tls_queue->earliest = expires_at;
    lk.unlock();
    WakeWorkerIfNeeded(expires_at);
  }
}

void TimerWorker::WaitForWorkers() { latch.wait(); }

void TimerWorker::ReapThreadLocalQueues() {
  for (auto&& p : producers_) {
    ThreadLocalQueue::EntryList t;
    {
      std::scoped_lock _(p->lock);
      t.swap(p->timers);
      p->earliest = std::chrono::steady_clock::time_point::max();
    }
    while (!t.empty()) {
      auto* e = t.pop_front();
      TRPC_DCHECK_NE(e, nullptr, "timer entry open nullptr");
      if (e->cancelled.load(std::memory_order_relaxed)) {
        e->DecrCount();
        continue;
      }
      auto ep = object_pool::LwSharedPtr(object_pool::lw_shared_adopt_ptr, e);
      timers_.push(std::move(ep));
    }
  }
}

void TimerWorker::FireTimers() {
  auto now = ReadSteadyClock();
  while (!timers_.empty()) {
    auto&& e = timers_.top();
    if (e->cancelled.load(std::memory_order_relaxed)) {
      timers_.pop();
      continue;
    }
    if (e->expires_at > now) {
      break;
    }

    // This IS slow, but if you have many timers to actually *fire*, you're in
    // trouble anyway.
    std::unique_lock lk(e->lock);
    auto cb = std::move(e->cb);
    lk.unlock();
    if (cb) {
      // `timer_id` is, actually, pointer to `Entry`.
      cb(reinterpret_cast<std::uint64_t>(e.Get()));
    }  // The timer is cancel between we were testing `e->cancelled` and
       // grabbing `e->lock`.

    // If it's a periodic timer, add a new pending timer.
    if (e->periodic) {
      if (cb) {
        // CAUTION: Do NOT create a new `Entry` otherwise timer ID we returned
        // in `AddTimer` will be invalidated.
        auto cp = std::move(e);  // FIXME: This `std::move` has no effect.
        std::unique_lock cplk(cp->lock);
        if (!cp->cancelled) {
          cp->expires_at = cp->expires_at + cp->interval;
          cp->cb = std::move(cb);  // Move user's callback back.
          cplk.unlock();
          timers_.push(std::move(cp));
        }
      } else {
        TRPC_CHECK(e->cancelled.load(std::memory_order_relaxed));
      }
    }
    timers_.pop();
  }
}

void TimerWorker::WakeWorkerIfNeeded(
    std::chrono::steady_clock::time_point local_expires_at) {
  auto expires_at = local_expires_at.time_since_epoch();
  auto r = next_expires_at_.load(std::memory_order_relaxed);

  while (true) {
    if (r <= expires_at) {  // Nothing to do then.
      return;
    }
    // This lock is needed, otherwise we might call `notify_one` after
    // `WorkerProc` tested `next_expires_at_` but before it actually goes into
    // sleep, losing our notification.
    //
    // By grabbing this lock, either we call `notify_one` before `WorkerProc`
    // tests `next_expires_at_`, which is safe; or we call `notify_one` after
    // `WorkerProc` slept and successfully deliver the notification, which
    // is, again, safe.
    //
    // Note that we may NOT do `compare_exchange` first (without lock) and then
    // grab the lock, that way it's still possible to loss wake-ups. Need to
    // grab this lock each time we *try* to move `next_expires_at_` is
    // unfortunate, but this branch should be rare nonetheless.
    std::scoped_lock _(lock_);
    if (next_expires_at_.compare_exchange_weak(r, expires_at)) {
      // We moved `next_expires_at_` earlier, wake up the worker then.
      cv_.notify_one();
      return;
    }
    // `next_expires_at_` has changed, retry then.
  }
}

inline TimerWorker::ThreadLocalQueue* TimerWorker::GetThreadLocalQueue() {
  thread_local EntryPtr p = trpc::object_pool::MakeLwShared<Entry>();
  thread_local ThreadLocalQueue q;
  return &q;
}

bool TimerWorker::EntryPtrComp::operator()(const EntryPtr& p1,
                                           const EntryPtr& p2) const {
  // `std::priority_queue` orders elements in descending order.
  return p1->expires_at > p2->expires_at;
}

}  // namespace trpc::fiber::detail
