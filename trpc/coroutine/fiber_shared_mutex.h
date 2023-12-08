//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/shared_mutex.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>
#include <cinttypes>

#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/util/likely.h"

namespace trpc {

/// @brief Adaptive shared mutex primitive for both fiber and pthread context.
/// @note  Performance-wise, reader-writer lock does NOT perform well unless
///        your critical section is sufficient large. In certain cases, reader-writer
///        lock can perform worse than `Mutex`. If reader performance is critical to
///        you, consider using other methods (e.g., thread-local cache, hazard pointers,
///        FiberSeqLock, ...).
class FiberSharedMutex {
 public:
  /// @brief Lock / unlock in exclusive mode (writer-side).
  /// @note Implementation of write side is slow.
  void lock();
  bool try_lock();
  void unlock();

  /// @brief Lock / unlock in shared mode (reader-side).
  /// @note  Optimized for non-contending (with writer) case.
  void lock_shared();
  bool try_lock_shared();
  void unlock_shared();

 private:
  void WaitForRead();
  void WakeupWriter();

 private:
  inline static constexpr auto kMaxReaders = 0x3fff'ffff;

  // Positive if no writer pending. Negative if (exactly) one writer is waiting.
  std::atomic<std::int32_t> reader_quota_{kMaxReaders};
  std::atomic<std::int32_t> pending_readers_{0};

  // Synchronizes readers and writers.
  FiberMutex wakeup_lock_;  // Acquired after `writer_lock_` if both acquired.
  FiberConditionVariable reader_cv_;
  FiberConditionVariable writer_cv_;
  int newly_granted_readers_ = 0;

  // Resolves contention between writers. This guarantees us that no more than
  // one writer can wait on `readers_`.
  FiberMutex writer_lock_;
};

}  // namespace trpc
