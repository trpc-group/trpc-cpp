//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/latch.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_latch.h"

namespace trpc {

FiberLatch::FiberLatch(std::ptrdiff_t count) : count_(count) {}

void FiberLatch::CountDown(std::ptrdiff_t update) {
  std::scoped_lock _(lock_);
  TRPC_CHECK_GE(count_, update);
  count_ -= update;
  if (!count_) {
    cv_.notify_all();
  }
}

bool FiberLatch::TryWait() const noexcept {
  std::scoped_lock _(lock_);
  TRPC_CHECK_GE(count_, 0);
  return !count_;
}

void FiberLatch::Wait() const {
  std::unique_lock lk(lock_);
  TRPC_CHECK_GE(count_, 0);
  cv_.wait(lk, [&] { return count_ == 0; });
}

void FiberLatch::ArriveAndWait(std::ptrdiff_t update) {
  CountDown(update);
  Wait();
}

}  // namespace trpc
