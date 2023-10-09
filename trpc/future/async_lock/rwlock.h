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

#include "trpc/future/async_lock/semaphore.h"

namespace trpc {

class RWLockForRead;
class RWLockForWrite;

/// @brief Read-write lock based on asynchronouse semaphore,
///        which write lock takes all available units.
/// @note It can not be used cross threads (not thread-safe).
class RWLock {
 public:
  RWLock() : sem_(max_ops_) {}

  ~RWLock() {}

  /// @brief Cast RWLock into read lock object that offers Lock & Unlock methods,
  ///        which is compatible with WithLock().
  /// @return Reference to RWLockForRead object.
  RWLockForRead* ForRead() { return reinterpret_cast<RWLockForRead*>(this); }

  /// @brief Cast RWLock into write lock object that offers Lock & Unlock methods,
  ///        which is compatible with WithLock().
  /// @return Reference to RWLockForWrite object.
  RWLockForWrite* ForWrite() { return reinterpret_cast<RWLockForWrite*>(this); }

  /// @brief Acquire a read lock.
  /// @return Future<> Ready:     lock object is free or only locked by readers.
  ///                  Not-ready: lock object is locked by a writer.
  ///                  Failed:    semaphore is broken, or current worker thread is not
  ///                             the same as the one that lock object was created on.
  Future<> ReadLock() { return sem_.Wait(); }

  /// @brief Release a read lock, and check wait list to see if there is any
  ///        waiter can be served.
  /// @note If it does not pass thread check, this method will do nothing, which
  ///       means it still locked. In case it's not unlocked properly, WithLock() is recommended.
  /// @return Whether unlock succeeded.
  bool ReadUnLock() { return sem_.Signal(); }

  /// @brief Acquire a write lock.
  /// @return Future<> Ready:     lock object is free.
  ///                  Not-ready: lock object is locked by a writer/reader.
  ///                  Failed:    semaphore is broken, or current worker thread is not
  ///                             the same as the one that lock object was created on.
  Future<> WriteLock() { return sem_.Wait(max_ops_); }

  /// @brief Release a write lock, and check wait list to see if there is any
  ///        waiter can be served.
  /// @note If it does not pass thread check, this method will do nothing, which
  ///       means it still locked. In case it's not unlocked properly, WithLock() is recommended.
  /// @return Whether unlock succeeded.
  bool WriteUnLock() { return sem_.Signal(max_ops_); }

  /// @brief Try to aquire a read lock without waiting.
  /// @return If it succeeded.
  bool TryReadLock() { return sem_.TryWait(); }

  /// @brief Try to aquire a write lock without waiting.
  /// @return If it succeeded.
  bool TryWriteLock() { return sem_.TryWait(max_ops_); }

  /// @brief Ask for how many waiters in wait lists.
  /// @return Count of waiters.
  size_t Waiters() { return sem_.Waiters(); }

 private:
  static constexpr size_t max_ops_ = Semaphore::MaxCounter();

  Semaphore sem_;
};

class RWLockForRead : private RWLock {
 public:
  Future<> Lock() { return static_cast<RWLock*>(this)->ReadLock(); }

  bool UnLock() { return static_cast<RWLock*>(this)->ReadUnLock(); }

  RWLockForRead& operator=(const RWLockForRead&) = delete;
};

class RWLockForWrite : private RWLock {
 public:
  Future<> Lock() { return static_cast<RWLock*>(this)->WriteLock(); }

  bool UnLock() { return static_cast<RWLock*>(this)->WriteUnLock(); }
};

}  // namespace trpc
