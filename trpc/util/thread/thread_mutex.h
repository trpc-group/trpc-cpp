//
//
// Copyright (C) 2016Tencent. All rights reserved.
// TarsCpp is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/TarsCloud/TarsCpp/blob/v1.0.0/util/include/util/tc_thread_mutex.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <mutex>
#include <string>

#include "trpc/util/thread/thread_cond.h"
#include "trpc/util/thread/thread_lock.h"

namespace trpc {

/// @brief Non-reentrant locking, meaning the same thread cannot lock again.
/// @note It is usually not used directly, but in conjunction with ThreadMonitor, i.e. ThreadLock.
class ThreadMutex {
 public:
  ThreadMutex() {}
  virtual ~ThreadMutex() {}

  // noncopyable
  ThreadMutex(const ThreadMutex&) = delete;
  void operator=(const ThreadMutex&) = delete;

  /// @brief Locking
  void Lock() const { mutex_.lock(); }

  /// @brief Try to lock
  /// @return true: locking successful; false: locking failed
  bool TryLock() const { return mutex_.try_lock(); }

  /// @brief Unlocking
  void Unlock() const { mutex_.unlock(); }

 protected:
  // Count
  int Count() const { return 0; }

  // Count with param
  void Count(int c) const {}

  friend class ThreadCond;

 protected:
  mutable std::mutex mutex_;
};

/// @brief Thread recursive lock class.
/// @note Reentrant locking, meaning the same thread can lock again.
class ThreadRecMutex {
 public:
  ThreadRecMutex() : count_(0) {}

  virtual ~ThreadRecMutex() {
    while (count_) {
      Unlock();
    }
  }

  /// @brief Locking
  void Lock() const {
    mutex_.lock();
    count_++;
  }

  /// @brief Unlock
  void Unlock() const {
    mutex_.unlock();
    count_--;
  }

  /// @brief Try to lock, throw an exception if failed.
  /// @return true, locking successful; false: another thread has already locked.
  bool TryLock() const {
    bool is_success = mutex_.try_lock();
    if (is_success) {
      count_++;
    }
    return is_success;
  }

 protected:
  friend class ThreadCond;

  // Count
  int Count() const {
    int c = count_;
    count_ = 0;
    return c;
  }

  // Count
  void Count(int c) const { count_ = c; }

 private:
  // Mutex object
  mutable std::recursive_mutex mutex_;
  mutable int count_;
};

}  // namespace trpc
