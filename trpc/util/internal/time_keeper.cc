//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/time_keeper.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/time_keeper.h"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

#include "trpc/util/likely.h"

using namespace std::literals;

namespace trpc::internal {

std::atomic<bool> TimeKeeper::exited_{false};

TimeKeeper* TimeKeeper::Instance() {
  static NeverDestroyedSingleton<TimeKeeper> tk;
  return tk.Get();
}

void TimeKeeper::Start() {
  worker_ = std::thread([&] { WorkerProc(); });
}

void TimeKeeper::Stop() {
  exited_.store(true, std::memory_order_relaxed);
  std::scoped_lock lk(lock_);
  cv_.notify_all();
}

void TimeKeeper::Join() {
  if (worker_.joinable()) {
    worker_.join();
  }
}

std::uint64_t TimeKeeper::AddTimer(std::chrono::steady_clock::time_point expires_at, std::chrono::nanoseconds interval,
                                   Function<void(std::uint64_t)> cb, bool is_slow_cb) {
  if (TRPC_UNLIKELY(exited_.load(std::memory_order_relaxed))) {
    return -1;
  }
  auto ptr = MakeRefCounted<Entry>();
  auto timer_id = reinterpret_cast<std::uint64_t>(ptr.Get());
  ptr->cb = std::make_shared<Function<void(std::uint64_t)>>(std::move(cb));
  ptr->cancelled.store(false, std::memory_order_relaxed);
  ptr->is_slow_cb = is_slow_cb;
  ptr->expires_at = std::max(std::chrono::steady_clock::now(), expires_at);
  ptr->interval = interval;
  ptr->Ref();  // For return value.

  std::scoped_lock lk(lock_);
  timers_.push(std::move(ptr));
  cv_.notify_all();  // Always wake worker up, performance does not matter here.

  return timer_id;
}

void TimeKeeper::KillTimer(std::uint64_t timer_id) {
  RefPtr<Entry> ref(adopt_ptr, reinterpret_cast<Entry*>(timer_id));
  std::scoped_lock lk(ref->lock);
  ref->cb = nullptr;
  ref->cancelled.store(true, std::memory_order_relaxed);
}

void TimeKeeper::WorkerProc() {
  while (!exited_.load(std::memory_order_relaxed)) {
    std::unique_lock lk(lock_);
    auto pred = [&] {
      return (!timers_.empty() && timers_.top()->expires_at <= std::chrono::steady_clock::now()) ||
             exited_.load(std::memory_order_relaxed);
    };

    auto next = timers_.empty() ? std::chrono::steady_clock::now() + 100s : timers_.top()->expires_at;
    cv_.wait_until(lk, next, pred);

    if (exited_.load(std::memory_order_relaxed)) {
      break;
    }
    if (!pred()) {
      continue;
    }

    // Fire the first timer.
    auto p = timers_.top();
    timers_.pop();

    lk.unlock();
    FireTimer(std::move(p));
  }
}

// Called with `lock_` held.
void TimeKeeper::FireTimer(EntryPtr ptr) {
  std::shared_ptr<Function<void(std::uint64_t)>> cb;
  {
    std::scoped_lock _(ptr->lock);
    cb = ptr->cb;
    if (ptr->cancelled.load(std::memory_order_relaxed)) {
      return;
    }
  }
  (*cb)(reinterpret_cast<std::uint64_t>(ptr.Get()));
  // Push it back with new `expires_at`.
  {
    std::scoped_lock _(ptr->lock);
    if (ptr->cancelled.load(std::memory_order_relaxed)) {
      // No need to add it back then.
      return;
    }
    ptr->expires_at += ptr->interval;
  }

  std::scoped_lock _(lock_);
  timers_.push(std::move(ptr));
}

}  // namespace trpc::internal
