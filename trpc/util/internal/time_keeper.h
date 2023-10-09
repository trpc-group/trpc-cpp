//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/time_keeper.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "trpc/util/internal/never_destroyed.h"
#include "trpc/util/function.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::internal {

// Several components in `fiber/base` need to do their bookkeeping periodically.
// So as not to introduce a timer manager for each of them, we use this class to
// serve them all.
//
// Note that in general, components in `fiber/base` should not use timer in
// `flare/fiber`. For other components, or user code, consider using fiber timer
// instead, this one is unlikely what you should use.
//
// This class performs poorly.
//
// NOT INTENDED FOR PUBLIC USE.
class TimeKeeper {
 public:
  static TimeKeeper* Instance();

  // DO NOT CALL THEM YOURSELF.
  //
  // `trpc::Start(...)` will call them at appropriate time.
  void Start();
  void Stop();
  void Join();

  // Register a timer.
  //
  // If `is_slow_cb` is set, `cb` is called outside of timer thread. This helps
  // improving timeliness. (If this is the case, `cb` may be called
  // concurrently. We might address this issue in the future.)
  std::uint64_t AddTimer(std::chrono::steady_clock::time_point expires_at,
                         std::chrono::nanoseconds interval, Function<void(std::uint64_t)> cb,
                         bool is_slow_cb);

  void KillTimer(std::uint64_t timer_id);

 private:
  friend class NeverDestroyedSingleton<TimeKeeper>;
  struct Entry : RefCounted<Entry> {
    std::mutex lock;
    std::atomic<bool> cancelled{false};
    std::atomic<bool> is_slow_cb;
    std::atomic<bool> running{false};  // Applicable to slow callback only, to
                                       // avoid calls to `cb` concurrently.
    std::shared_ptr<Function<void(std::uint64_t)>> cb;
    std::chrono::steady_clock::time_point expires_at;
    std::chrono::nanoseconds interval;
  };
  using EntryPtr = RefPtr<Entry>;
  struct EntryPtrComp {
    bool operator()(const EntryPtr& x, const EntryPtr& y) const {
      return x->expires_at > y->expires_at;
    }
  };

  TimeKeeper() = default;
  TimeKeeper(const TimeKeeper&) = delete;
  TimeKeeper& operator=(const TimeKeeper&) = delete;

  void WorkerProc();
  void FireTimer(EntryPtr ptr);

 private:
  // We use it to workaround global destruction order issue.
  //
  // If set, `KillTimer` will be effectively a no-op.
  static std::atomic<bool> exited_;

  std::mutex lock_;
  std::condition_variable cv_;
  std::priority_queue<EntryPtr, std::vector<EntryPtr>, EntryPtrComp> timers_;
  std::thread worker_;
};

}  // namespace trpc::internal
