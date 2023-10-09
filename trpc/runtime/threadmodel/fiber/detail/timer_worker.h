//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/timer_worker.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "trpc/util/align.h"
#include "trpc/util/function.h"
#include "trpc/util/latch.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/doubly_linked_list.h"

namespace trpc {

template <class>
struct PoolTraits;

}  // namespace trpc

namespace trpc::fiber::detail {

class SchedulingGroup;

// This class contains a dedicated pthread for running timers.
class alignas(hardware_destructive_interference_size) TimerWorker {
 public:
  explicit TimerWorker(SchedulingGroup* sg, bool disable_process_name = true);
  ~TimerWorker();

  static TimerWorker* GetTimerOwner(std::uint64_t timer_id);

  // Create a timer. It's enabled separately via `EnableTimer`.
  //
  // @sa: `SchedulingGroup::CreateTimer`
  std::uint64_t CreateTimer(std::chrono::steady_clock::time_point expires_at,
                            Function<void(std::uint64_t)>&& cb);
  std::uint64_t CreateTimer(std::chrono::steady_clock::time_point initial_expires_at,
                            std::chrono::nanoseconds interval, Function<void(std::uint64_t)>&& cb);

  // Enable a timer created before.
  void EnableTimer(std::uint64_t timer_id);

  // Cancel a timer.
  void RemoveTimer(std::uint64_t timer_id);

  // Detach a timer. This method can be helpful in fire-and-forget use cases.
  void DetachTimer(std::uint64_t timer_id);

  SchedulingGroup* GetSchedulingGroup();

  // Caller MUST be one of the pthread workers belong to the same scheduling
  // group.
  void InitializeLocalQueue(std::size_t worker_index);

  // Start the worker thread and join the given scheduling group.
  void Start();

  // Stop & Join.
  void Stop();
  void Join();

  // Non-copyable, non-movable.
  TimerWorker(const TimerWorker&) = delete;
  TimerWorker& operator=(const TimerWorker&) = delete;

 private:
  struct Entry;
  using EntryPtr = object_pool::LwSharedPtr<Entry>;
  struct ThreadLocalQueue;

  void WorkerProc();

  void AddTimer(EntryPtr timer);

  // Wait until all worker has called `InitializeLocalQueue()`.
  void WaitForWorkers();

  void ReapThreadLocalQueues();
  void FireTimers();
  void WakeWorkerIfNeeded(std::chrono::steady_clock::time_point local_expires_at);

  static ThreadLocalQueue* GetThreadLocalQueue();

 private:
  struct EntryPtrComp {
    bool operator()(const EntryPtr& p1, const EntryPtr& p2) const;
  };
  friend struct PoolTraits<Entry>;

  SchedulingGroup* sg_;
  std::atomic<bool> stopped_{false};
  bool disable_process_name_{true};

  trpc::Latch latch;  // We use it to wait for workers' registration.

  std::vector<ThreadLocalQueue*> producers_;  // Pointer to thread-local timer
                                              // vectors.

  // Unfortunately GCC 8.2 does not support
  // `std::atomic<std::chrono::steady_clock::time_point>` (this is conforming,
  // as that specialization is not defined by the Standard), so we need to store
  // `time_point::time_since_epoch()` here.
  std::atomic<std::chrono::steady_clock::duration> next_expires_at_{
      std::chrono::steady_clock::duration::max()};
  std::priority_queue<EntryPtr, std::vector<EntryPtr>, EntryPtrComp> timers_;

  std::thread worker_;

  // `WorkerProc` sleeps on this.
  std::mutex lock_;
  std::condition_variable cv_;
};

}  // namespace trpc::fiber::detail
