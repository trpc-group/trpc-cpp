//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/memory_barrier.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/internal/memory_barrier.h"

#include <sys/mman.h>

#include <mutex>

#include "trpc/util/check.h"
#include "trpc/util/internal/never_destroyed.h"
#include "trpc/util/log/logging.h"

namespace trpc::internal {

namespace {

/// @brief `membarrier()` is not usable until Linux 4.3, which is not available until
///         tlinux 3 is deployed.
///
/// Here Folly presents a workaround by mutating our page tables, which
/// implicitly cause the system to execute a barrier on every core running our
/// threads. I shamelessly copied their solution here.
///
/// @sa: https://github.com/facebook/folly/blob/master/folly/synchronization/AsymmetricMemoryBarrier.cpp

void* dummy_page = [] {
  auto ptr = mmap(nullptr, 1, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  TRPC_PCHECK(ptr, "Cannot create dummy page for asymmetric memory barrier.");
  (void)mlock(ptr, 1);
  return ptr;
}();  // Never freed.

void HomemadeMembarrier() {
  // Previous memory accesses may not be reordered after syscalls below.
  // (Frankly this is implied by acquired lock below.)
  MemoryBarrier();

  static NeverDestroyed<std::mutex> lock;
  std::scoped_lock _(*lock);

  // Upgrading protection does not always result in fence in each core (as it
  // can be delayed until #PF).
  TRPC_PCHECK(mprotect(dummy_page, 1, PROT_READ | PROT_WRITE) == 0);
  *static_cast<char*>(dummy_page) = 0;  // Make sure the page is present.
  // This time a barrier should be issued to every cores.
  TRPC_PCHECK(mprotect(dummy_page, 1, PROT_READ) == 0);

  // Subsequent memory accesses may not be reordered before syscalls above. (Not
  // sure if it's already implied by `mprotect`?)
  MemoryBarrier();
}

}  // namespace

void AsymmetricBarrierHeavy() {
  HomemadeMembarrier();
}

}  // namespace trpc::internal
