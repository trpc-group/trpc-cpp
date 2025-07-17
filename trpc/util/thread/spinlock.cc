//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/spinlock.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/spinlock.h"

#include "trpc/util/thread/compile.h"

namespace trpc {

/// @s: https://code.woboq.org/userspace/glibc/nptl/pthread_spin_lock.c.html
void Spinlock::LockSlow() noexcept {
  do {
    while (locked_.load(std::memory_order_relaxed)) {
      TRPC_CPU_RELAX();
    }
  } while (locked_.exchange(true, std::memory_order_acquire));
}

}  // namespace trpc
