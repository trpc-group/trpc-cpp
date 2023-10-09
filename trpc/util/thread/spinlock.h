//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/spinlock.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>

#include "trpc/util/likely.h"

namespace trpc {

/// @brief SpinLock implementation class.
class Spinlock {
 public:
  /// @brief Locking operation.
  void lock() noexcept {
    // Here we try to grab the lock first before failing back to TTAS.
    //
    // If the lock is not contend, this fast-path should be quicker.
    // If the lock is contended and we have to fail back to slow TTAS, this
    // single try shouldn't add too much overhead.
    //
    // What's more, by keeping this method small, chances are higher that this
    // method get inlined by the compiler.
    if (TRPC_LIKELY(try_lock())) {
      return;
    }

    // Slow path otherwise.
    LockSlow();
  }

  /// @brief Try to acquire the lock.
  /// @return true: if the lock was acquired successfully; false: otherwise
  bool try_lock() noexcept { return !locked_.exchange(true, std::memory_order_acquire); }

  /// @brief Unlock
  void unlock() noexcept { locked_.store(false, std::memory_order_release); }

 private:
  void LockSlow() noexcept;

 private:
  std::atomic<bool> locked_{false};
};

}  // namespace trpc
