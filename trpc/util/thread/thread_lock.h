//
//
// Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
// TarsCpp is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/TarsCloud/TarsCpp/blob/v1.0.0/util/include/util/tc_lock.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cerrno>
#include <stdexcept>
#include <string>

namespace trpc {

/// @brief The lock template class is used in conjunction with other specific locks.
///        It is locked during construction and unlocked during destruction.
template <typename T>
class ThreadLockT {
 public:
  // Lock during construction.
  explicit ThreadLockT(const T& mutex) : mutex_(mutex) {
    mutex_.Lock();
    acquired_ = true;
  }

  // Unlock during destruction.
  virtual ~ThreadLockT() {
    if (acquired_) {
      mutex_.Unlock();
    }
  }

  // Not implemented; prevents accidental use.
  ThreadLockT(const ThreadLockT&) = delete;
  ThreadLockT& operator=(const ThreadLockT&) = delete;

  /// @brief Acquire the lock.
  /// @return true: locking succeeded; false: locking failed.
  bool Acquire() const {
    if (acquired_) {
      return false;
    }
    mutex_.Lock();
    acquired_ = true;
    return true;
  }

  /// @brief Try to acquire the lock.
  /// @return true: locking succeeded; false: locking failed.
  bool TryAcquire() const {
    acquired_ = mutex_.TryLock();
    return acquired_;
  }

  /// @brief Release the lock.
  /// @return false: lock has not been acquired; true: unlocking succeeded.
  bool Release() const {
    if (!acquired_) {
      return false;
    }
    mutex_.Unlock();
    acquired_ = false;
  }

  /// @brief Check if the lock has been acquired.
  /// @return true: lock has been acquired; false: lock has not been acquired
  bool Acquired() const { return acquired_; }

 protected:
  // Used for lock attempt operations, similar to ThreadLockT.
  ThreadLockT(const T& mutex, bool) : mutex_(mutex) { acquired_ = mutex_.TryLock(); }

 protected:
  // Lock object.
  const T& mutex_;

  // Whether the lock has been acquired.
  mutable bool acquired_;
};

/// @brief Class for attempting to acquire a lock.
template <typename T>
class ThreadTryLockT : public ThreadLockT<T> {
 public:
  explicit ThreadTryLockT(const T& mutex) : ThreadLockT<T>(mutex, true) {}
};

}  // namespace trpc
