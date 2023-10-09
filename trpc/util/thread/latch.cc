//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/latch.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/latch.h"

namespace trpc {

Latch::Latch(std::ptrdiff_t count) : count_(count) {}

void Latch::count_down(std::ptrdiff_t n) {
  std::unique_lock lk(m_);
  TRPC_CHECK_GE(count_, n);
  count_ -= n;
  if (!count_) {
    cv_.notify_all();
  }
}

bool Latch::try_wait() const noexcept {
  std::scoped_lock _(m_);
  TRPC_CHECK_GE(count_, 0);
  return !count_;
}

void Latch::wait() const {
  std::unique_lock lk(m_);
  TRPC_CHECK_GE(count_, 0);
  return cv_.wait(lk, [this] { return count_ == 0; });
}

void Latch::arrive_and_wait(std::ptrdiff_t n) {
  count_down(n);
  wait();
}

}  // namespace trpc
