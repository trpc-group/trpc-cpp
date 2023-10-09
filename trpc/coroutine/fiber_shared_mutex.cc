//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/shared_mutex.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_shared_mutex.h"

namespace trpc {

void FiberSharedMutex::lock_shared() {
  if (auto was = reader_quota_.fetch_sub(1, std::memory_order_acquire); TRPC_LIKELY(was > 1)) {
    TRPC_CHECK_LE(was, kMaxReaders);
  } else {
    TRPC_CHECK_NE(was, 1);
    return WaitForRead();
  }
}

bool FiberSharedMutex::try_lock_shared() {
  auto was = reader_quota_.load(std::memory_order_relaxed);
  do {
    TRPC_CHECK_LE(was, kMaxReaders);
    TRPC_CHECK_NE(was, 1);
    if (was <= 0) {
      return false;
    }
  } while (!reader_quota_.compare_exchange_weak(was, was - 1, std::memory_order_acquire));
  return true;
}

void FiberSharedMutex::unlock_shared() {
  if (auto was = reader_quota_.fetch_add(1, std::memory_order_release); TRPC_LIKELY(was > 0)) {
    // No writer is waiting, nothing to do.
    TRPC_CHECK_LT(was, kMaxReaders);
  } else {
    if (pending_readers_.fetch_sub(1, std::memory_order_release) == 1) {
      return WakeupWriter();
    }
  }
}

void FiberSharedMutex::lock() {
  // There can be at most one active writer at a time.
  writer_lock_.lock();  // Unlocked in `unlock()`.
  auto was = reader_quota_.fetch_sub(kMaxReaders, std::memory_order_acquire);
  if (was == kMaxReaders) {
    // No active readers.
    return;
  }
  TRPC_CHECK_LT(was, kMaxReaders);
  TRPC_CHECK_GT(was, 0);

  // Wait until all existing readers (but not the new-comers) finish their job.
  std::unique_lock lk(wakeup_lock_);
  auto pending_readers = kMaxReaders - was;
  auto new_pending_readers = pending_readers_.fetch_add(pending_readers, std::memory_order_acquire);
  if (new_pending_readers != -pending_readers) {
    writer_cv_.wait(lk, [&] { return pending_readers_.load(std::memory_order_relaxed) == 0; });
  }
  return;
}

bool FiberSharedMutex::try_lock() {
  std::unique_lock lk(writer_lock_, std::try_to_lock);
  if (!lk) {
    return false;
  }
  auto was = reader_quota_.load(std::memory_order_relaxed);
  do {
    if (was != kMaxReaders) {  // Active readers out there.
      return false;
    }
  } while (!reader_quota_.compare_exchange_weak(was, 0, std::memory_order_acquire));
  lk.release();  // It's unlocked in `unlock()`.
  return true;
}

void FiberSharedMutex::unlock() {
  auto was = reader_quota_.fetch_add(kMaxReaders, std::memory_order_release);
  TRPC_CHECK_GT(was + kMaxReaders, 0);  // Underflow then.
  // No writer in? It can't be. We're still holding the lock.
  TRPC_CHECK_LE(was, 0);
  if (was != 0) {
    std::unique_lock lk(wakeup_lock_);
    // Unblock all pending readers. (Note that it's possible that a new-comer is
    // "unblocked" by this variable, and starve an old reader. Given that writer
    // should be rare, this shouldn't hurt much.).
    newly_granted_readers_ = -was;
    // Readers waiting.
    lk.unlock();
    reader_cv_.notify_all();
  }
  writer_lock_.unlock();  // Allow other writers to come in.
}

void FiberSharedMutex::WaitForRead() {
  std::unique_lock lk(wakeup_lock_);
  reader_cv_.wait(lk, [&] {
    if (newly_granted_readers_) {  // Writer has gone.
      --newly_granted_readers_;
      return true;
    }
    return false;
  });
}

void FiberSharedMutex::WakeupWriter() {
  wakeup_lock_.lock();
  wakeup_lock_.unlock();
  writer_cv_.notify_one();
}

}  // namespace trpc
