//
//
// Copyright (C) 2018-2022 Dr. Tsung-Wei Huang
// taskflow is licensed under the MIT License.
// The source codes in this file based on
// https://github.com/taskflow/taskflow/blob/v3.5.0/taskflow/core/notifier.hpp.
// This source file may have been modified by THL A29 Limited, and licensed under the MIT License.
//
//

#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

namespace trpc {

/// @brief PredicateNotifier allows to wait for arbitrary predicates in non-blocking
///       algorithms. Think of condition variable, but wait predicate does not need to
///       be protected by a mutex. Usage:
///       Waiting thread does:
///
///         if (predicate)
///           return act();
///         PredicateNotifier::Waiter& w = waiters[my_index];
///         ec.PrepareWait(&w);
///         if (predicate) {
///           ec.CancelWait(&w);
///           return act();
///         }
///         ec.CommitWait(&w);
///
///       Notifying thread does:
///
///         predicate = true;
///         ec.notify(true);
///
/// @note notify is cheap if there are no waiting threads. PrepareWait/CommitWait are not
///       cheap, but they are executed only if the preceeding predicate check has failed.
///
/// @details Algorihtm outline: There are two main variables: predicate (managed by user) and state_.
///          Operation closely resembles Dekker mutual algorithm @sa https://en.wikipedia.org/wiki/Dekker%27s_algorithm
///          Waiting thread sets state_ then checks predicate, Notifying thread set spredicate then checks state_.
///          Due to seq_cst fences in between these operations it is guaranteed than either waiter will see predicate
///          change and won't block, or notifying thread will see state_ change and will unblock the waiter, or both.
///          But it can't happen that both threads don't see each other changes, which would lead to deadlock.
class PredicateNotifier {
 public:
  /// @brief waiting notify
  struct Waiter {
    std::atomic<Waiter*> next;
    std::mutex mu;
    std::condition_variable cv;
    uint64_t epoch;
    unsigned state;
    enum {
      kNotSignaled,
      kWaiting,
      kSignaled,
    };
  };

  explicit PredicateNotifier(size_t N) : waiters_{N} {
    assert(waiters_.size() < (1 << kWaiterBits) - 1);
    // Initialize epoch to something close to overflow to test overflow.
    state_ = kStackMask | (kEpochMask - kEpochInc * waiters_.size() * 2);
  }

  ~PredicateNotifier() {
    // Ensure there are no waiters.
    assert((state_.load() & (kStackMask | kWaiterMask)) == kStackMask);
  }

  /// @brief PrepareWait prepares for waiting.
  /// @param w waiter
  /// @note After calling this function the thread must re-check the wait predicate
  ///       and call either CancelWait or CommitWait passing the same Waiter object.
  void PrepareWait(Waiter* w) {
    w->epoch = state_.fetch_add(kWaiterInc, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

  /// @brief CommitWait commits waiting.
  /// @param w waiter
  void CommitWait(Waiter* w) {
    w->state = Waiter::kNotSignaled;
    // Modification epoch of this waiter.
    uint64_t epoch = (w->epoch & kEpochMask) + (((w->epoch & kWaiterMask) >> kWaiterShift) << kEpochShift);
    uint64_t state = state_.load(std::memory_order_seq_cst);
    for (;;) {
      if (int64_t((state & kEpochMask) - epoch) < 0) {
        // The preceeding waiter has not decided on its fate. Wait until it
        // calls either CancelWait or CommitWait, or is notified.
        std::this_thread::yield();
        state = state_.load(std::memory_order_seq_cst);
        continue;
      }
      // We've already been notified.
      if (int64_t((state & kEpochMask) - epoch) > 0) return;
      // Remove this thread from prewait counter and add it to the waiter list.
      assert((state & kWaiterMask) != 0);
      uint64_t newstate = state - kWaiterInc + kEpochInc;
      // newstate = (newstate & ~kStackMask) | (w - &waiters_[0]);
      newstate = static_cast<uint64_t>((newstate & ~kStackMask) | static_cast<uint64_t>(w - &waiters_[0]));
      if ((state & kStackMask) == kStackMask)
        w->next.store(nullptr, std::memory_order_relaxed);
      else
        w->next.store(&waiters_[state & kStackMask], std::memory_order_relaxed);
      if (state_.compare_exchange_weak(state, newstate, std::memory_order_release)) break;
    }
    Park(w);
  }

  /// @brief CancelWait cancels effects of the previous PrepareWait call.
  /// @param w waiter
  void CancelWait(Waiter* w) {
    uint64_t epoch = (w->epoch & kEpochMask) + (((w->epoch & kWaiterMask) >> kWaiterShift) << kEpochShift);
    uint64_t state = state_.load(std::memory_order_relaxed);
    for (;;) {
      if (int64_t((state & kEpochMask) - epoch) < 0) {
        // The preceeding waiter has not decided on its fate. Wait until it
        // calls either CancelWait or CommitWait, or is notified.
        std::this_thread::yield();
        state = state_.load(std::memory_order_relaxed);
        continue;
      }
      // We've already been notified.
      if (int64_t((state & kEpochMask) - epoch) > 0) return;
      // Remove this thread from prewait counter.
      assert((state & kWaiterMask) != 0);
      if (state_.compare_exchange_weak(state, state - kWaiterInc + kEpochInc, std::memory_order_relaxed)) return;
    }
  }

  /// @brief notify wakes one or all waiting threads.
  /// @param all whether or not all
  /// @note  Must be called after changing the associated wait predicate.
  void Notify(bool all) {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    uint64_t state = state_.load(std::memory_order_acquire);
    for (;;) {
      // Easy case: no waiters.
      if ((state & kStackMask) == kStackMask && (state & kWaiterMask) == 0) return;
      uint64_t waiters = (state & kWaiterMask) >> kWaiterShift;
      uint64_t newstate;
      if (all) {
        // Reset prewait counter and empty wait list.
        newstate = (state & kEpochMask) + (kEpochInc * waiters) + kStackMask;
      } else if (waiters) {
        // There is a thread in pre-wait state, unblock it.
        newstate = state + kEpochInc - kWaiterInc;
      } else {
        // Pop a waiter from list and unpark it.
        Waiter* w = &waiters_[state & kStackMask];
        Waiter* wnext = w->next.load(std::memory_order_relaxed);
        uint64_t next = kStackMask;
        // if (wnext != nullptr) next = wnext - &waiters_[0];
        if (wnext != nullptr) next = static_cast<uint64_t>(wnext - &waiters_[0]);
        // Note: we don't add kEpochInc here. ABA problem on the lock-free stack
        // can't happen because a waiter is re-pushed onto the stack only after
        // it was in the pre-wait state which inevitably leads to epoch
        // increment.
        newstate = (state & kEpochMask) + next;
      }
      if (state_.compare_exchange_weak(state, newstate, std::memory_order_acquire)) {
        if (!all && waiters) return;  // unblocked pre-wait thread
        if ((state & kStackMask) == kStackMask) return;
        Waiter* w = &waiters_[state & kStackMask];
        if (!all) w->next.store(nullptr, std::memory_order_relaxed);
        Unpark(w);
        return;
      }
    }
  }

  /// @brief notify n waiters
  /// @param n the number of waiters
  void NotifyN(size_t n) {
    if (n >= waiters_.size()) {
      Notify(true);
    } else {
      for (size_t k = 0; k < n; ++k) {
        Notify(false);
      }
    }
  }

  /// @brief  get the number of waiters
  /// @return the number of waiters
  size_t Size() const { return waiters_.size(); }

  /// @brief get waiter idx
  /// @return Waiter
  Waiter& GetWaiter(size_t idx) { return waiters_[idx]; }

 private:
  // State_ layout:
  // - low kStackBits is a stack of waiters committed wait.
  // - next kWaiterBits is count of waiters in prewait state.
  // - next kEpochBits is modification counter.
  static const uint64_t kStackBits = 16;
  static const uint64_t kStackMask = (1ull << kStackBits) - 1;
  static const uint64_t kWaiterBits = 16;
  static const uint64_t kWaiterShift = 16;
  static const uint64_t kWaiterMask = ((1ull << kWaiterBits) - 1) << kWaiterShift;
  static const uint64_t kWaiterInc = 1ull << kWaiterBits;
  static const uint64_t kEpochBits = 32;
  static const uint64_t kEpochShift = 32;
  static const uint64_t kEpochMask = ((1ull << kEpochBits) - 1) << kEpochShift;
  static const uint64_t kEpochInc = 1ull << kEpochShift;
  std::atomic<uint64_t> state_;
  std::vector<Waiter> waiters_;

  void Park(Waiter* w) {
    std::unique_lock<std::mutex> lock(w->mu);
    while (w->state != Waiter::kSignaled) {
      w->state = Waiter::kWaiting;
      w->cv.wait(lock);
    }
  }

  void Unpark(Waiter* waiters) {
    Waiter* next = nullptr;
    for (Waiter* w = waiters; w; w = next) {
      next = w->next.load(std::memory_order_relaxed);
      unsigned state;
      {
        std::unique_lock<std::mutex> lock(w->mu);
        state = w->state;
        w->state = Waiter::kSignaled;
      }
      // Avoid notifying if it wasn't waiting.
      if (state == Waiter::kWaiting) w->cv.notify_one();
    }
  }
};

}  // namespace trpc
