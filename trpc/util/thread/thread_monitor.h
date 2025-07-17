//
//
// Copyright (C) 2016Tencent. All rights reserved.
// TarsCpp is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/TarsCloud/TarsCpp/blob/v1.0.0/util/include/util/tc_monitor.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/util/thread/thread_cond.h"
#include "trpc/util/thread/thread_mutex.h"

namespace trpc {

/// @brief Usually, thread locks are used through this class, i.e. ThreadLock and ThreadRecLock,
///        rather than directly using ThreadMutex and ThreadRecMutex.
///        Moreover, this class combines ThreadMutex/ThreadRecMutex with ThreadCond to be used for the wake-up and wait
///        behavior between threads in the producer-consumer-edge scenario.
template <class T, class P>
class ThreadMonitor {
 public:
  /// @brief Define a lock control object.
  using LockT = ThreadLockT<ThreadMonitor<T, P> >;
  using TryLockT = ThreadTryLockT<ThreadMonitor<T, P> >;

  ThreadMonitor() = default;
  virtual ~ThreadMonitor() {}

  // noncopyable
  ThreadMonitor(const ThreadMonitor&) = delete;
  void operator=(const ThreadMonitor&) = delete;

  /// @brief Locking. At this point, other threads calling Lock will be blocked.
  void Lock() const { mutex_.Lock(); }

  /// @brief Unlocking. At this point, other threads will contend for the lock.
  void Unlock() const { mutex_.Unlock(); }

  /// @brief Try to acquire the lock.
  /// @return true: success; false: failure.
  bool TryLock() const { return mutex_.TryLock(); }

  /// @brief Waiting. The current calling thread waits on the lock until an event notification is received,
  ///        and then locks and continues execution.
  /// @note Before executing this function, Lock() needs to be executed to lock. When the thread is awakened,
  ///       this function will request the lock. Therefore, after executing Wait, the lock needs to be released.
  void Wait() const {
    cond_.Wait(mutex_);
    Lock();
  }

  /// @brief Waiting. The current calling thread waits on the lock until a timeout or being awakened,
  ///        and then locks and continues execution.
  /// @param millsecond Waiting time.
  /// @return false: timeout; true: an event occurred.
  /// @note Before executing this function, Lock() needs to be executed to lock. When the thread times out
  ///       or is awakened, this function will request the lock. Therefore, after executing TimedWait,
  //        the lock needs to be released.
  bool TimedWait(int millsecond) const {
    bool rc = cond_.TimedWait(mutex_, millsecond);
    Lock();
    return rc;
  }

  /// @brief Notify one thread waiting on this lock to wake up.
  void Notify() { cond_.Signal(); }

  /// @brief Notify all threads waiting on this lock to wake up.
  void NotifyAll() { cond_.Broadcast(); }

 protected:
  mutable P cond_;
  T mutex_;
};

// Regular thread lock.
using ThreadLock = ThreadMonitor<ThreadMutex, ThreadCond>;

// Recursive lock (a thread can acquire the lock multiple times at different levels).
using ThreadRecLock = ThreadMonitor<ThreadRecMutex, ThreadCond>;

}  // namespace trpc
