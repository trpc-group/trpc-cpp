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

#include <linux/futex.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <memory>

namespace trpc {

/// @brief Waking up/notifying between multiple threads based on futex
/// reference implementation: https://github.com/apache/incubator-brpc/blob/master/src/bthread/parking_lot.h
class FutexNotifier {
 public:
  class State {
   public:
    State(): val_(0) {}
    bool Stopped() const { return val_ & 1; }
   private:
    friend class FutexNotifier;
    explicit State(int val) : val_(val) {}
    int val_;
  };

  FutexNotifier() : pending_wake_(0) {}

  int Wake(int num) {
    pending_wake_.fetch_add((num << 1), std::memory_order_release);
    return syscall(SYS_futex, &pending_wake_, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG),
                   num, NULL, NULL, 0);
  }

  State GetState() {
    return State(pending_wake_.load(std::memory_order_acquire));
  }

  void Wait(const State& expected_state) {
    syscall(SYS_futex, &pending_wake_, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG),
            expected_state.val_, NULL, NULL, 0);
  }

  void Stop() {
    pending_wake_.fetch_or(1);
    syscall(SYS_futex, &pending_wake_, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG),
            10000, NULL, NULL, 0);
  }

 private:
  std::atomic<int> pending_wake_;
};

}  // namespace trpc
