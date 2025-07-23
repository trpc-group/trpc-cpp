//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <type_traits>

#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/util/thread/compile.h"
#include "trpc/util/thread/memory_barrier.h"
#include "trpc/util/likely.h"

namespace trpc {

/// @brief Reader/writer consistent mechanism without starving writers.
///        This type of lock for data where the reader wants a consistent set of information
///        and is willing to retry if the information changes.
///        Readers never block but they may have to retry if a writer is in progress.
///        Writers do not wait for readers.
///        This will not work for data that contains pointers, because any writer could
///        invalidate a pointer that a reader was following.
///        Read first: https://en.wikipedia.org/wiki/Seqlock
/// @code
///        Reader usage:
///        FiberSeqLoc seqlock;
///        do {
///          seq = seqlock.ReadSeqBegin();
///          ...
///        } while (seqlock.ReadSeqRetry(seq));
///
///        Writer usage:
///        seqlock.WriteSeqLock();
///        ...
///        seqlock.WriteSeqUnlock();
/// @endcode
class FiberSeqLock {
 public:
  /// @brief Reader-side functions for starting and finalizing a read side section.
  unsigned ReadSeqBegin();

  /// @brief Retry read entry.
  unsigned ReadSeqRetry(unsigned retry_num);

  /// @brief Lock out other writers and update the count.
  ///        Acts like a normal unilock lock/unlock.
  void WriteSeqLock();

  /// @brief Unlock
  void WriteSeqUnlock();

 private:
  // Version using sequence counter only.
  // This can be used when code has its own mutex protecting the
  // updating starting before the `ReadSeqCountBegin` and ending
  // after `WriteSeqCountEnd`.
  struct Seqcount {
    unsigned sequence;
  };

  struct Seqlock {
    struct Seqcount seqcount;
    FiberMutex lock;
  };

  // Begin a seq-read critical section (without barrier)
  //
  // Callers should ensure ordering is provided before actually loading any of
  // the variables that are to be protected in this critical section.
  //
  // Use carefully, only in critical code, and comment how the barrier is provided.
  inline unsigned ReadSeqCountBlock(const Seqcount* s);

  // Begin a seq-read critical section
  // Return count to be passed to read_seqcount_retry
  //
  // `ReadSeqCountBegin` opens a read critical section of the given seqcount.
  // Validity of the critical section is tested by checking `ReadSeqCountRetry` function.
  inline unsigned ReadSeqCountBegin(const Seqcount* s);

  // End a seq-read critical section (without barrier)
  // `retry_num`: from read_seqcount_begin
  // Return: 1 if retry is required, else 0
  //
  // Like ReadSeqCountRetry, but has no barrier.
  // Callers should ensure ordering is provided before actually loading any of
  // the variables that are to be protected in this critical section.
  //
  // Use carefully, only in critical code, and comment how the barrier is provided.
  inline unsigned ReadSeqCountRetryAt(const Seqcount* s, unsigned retry_num);

  // End a seq-read critical section
  // `retry_num`: from read_seqcount_begin
  // Return: 1 if retry is required, else 0
  //
  // ReadSeqCountRetry closes a read critical section of the given seqcount.
  // If the critical section was invalid, it must be ignored (and typically retried).
  inline unsigned ReadSeqCountRetry(const Seqcount* s, unsigned retry_num);

  // Sequence counter only version assumes that callers are using their own mutexing.
  inline void WriteSeqCountBegin(Seqcount* s);

  inline void WriteSeqCountEnd(Seqcount* s);

 private:
  // seqlock impliment by FiberMutex and a sequence
  Seqlock sl_;
};

unsigned FiberSeqLock::ReadSeqCountBlock(const Seqcount* s) {
  unsigned ret;
  for (;;) {
    ret = TRPC_ACCESS_ONCE(s->sequence);
    if (TRPC_UNLIKELY(ret & 1)) {
      TRPC_CPU_RELAX();
    } else {
      break;
    }
  }
  return ret;
}

unsigned FiberSeqLock::ReadSeqCountBegin(const Seqcount* s) {
  unsigned ret = ReadSeqCountBlock(s);
  trpc::MemoryBarrier();
  return ret;
}

unsigned FiberSeqLock::ReadSeqCountRetryAt(const Seqcount* s, unsigned retry_num) {
  return TRPC_UNLIKELY(s->sequence != retry_num);
}

unsigned FiberSeqLock::ReadSeqCountRetry(const Seqcount* s, unsigned retry_num) {
  trpc::MemoryBarrier();
  return ReadSeqCountRetryAt(s, retry_num);
}

void FiberSeqLock::WriteSeqCountBegin(Seqcount* s) {
  s->sequence++;
  trpc::MemoryBarrier();
}

void FiberSeqLock::WriteSeqCountEnd(Seqcount* s) {
  trpc::MemoryBarrier();
  s->sequence++;
}

}  // namespace trpc
