//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/scheduling_group.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v1/run_queue.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/scheduling.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/util/align.h"
#include "trpc/util/check.h"
#include "trpc/util/delayed_init.h"
#include "trpc/util/function.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::fiber::detail::v1 {

/// @brief An implementation of scheduling based on flare/fiber.
class alignas(hardware_destructive_interference_size) SchedulingImpl final : public Scheduling {
 public:
  SchedulingImpl() = default;

  ~SchedulingImpl() override = default;

  bool Init(SchedulingGroup* scheduling_group,
            std::size_t scheduling_group_size) noexcept override;

  void AddForeignSchedulingGroup(std::size_t worker_index, SchedulingGroup* sg,
                                 std::uint64_t steal_every_n) noexcept override;

  FiberEntity* StealFiberFromForeignSchedulingGroup() noexcept override;
  FiberEntity* RemoteAcquireFiber() noexcept;

  void Enter(std::size_t index) noexcept override;

  void Schedule() noexcept override;

  void Leave() noexcept override;

  bool StartFiber(FiberDesc* desc) noexcept override;

  void StartFibers(FiberDesc** start, FiberDesc** end) noexcept override;

  void Suspend(FiberEntity* self, std::unique_lock<Spinlock>&& scheduler_lock) noexcept override;

  void Resume(FiberEntity* to) noexcept override;

  void Resume(FiberEntity* self, FiberEntity* to) noexcept override;

  void Yield(FiberEntity* self) noexcept override;

  void SwitchTo(FiberEntity* self, FiberEntity* to) noexcept override;

  void Stop() noexcept override;

  std::size_t GetFiberQueueSize() noexcept override;

 private:
  FiberEntity* AcquireFiber() noexcept;
  FiberEntity* SpinningAcquireFiber() noexcept;
  FiberEntity* WaitForFiber() noexcept;
  bool WakeUpOneWorker() noexcept;
  bool WakeUpWorkers(std::size_t n) noexcept;
  bool WakeUpOneSpinningWorker() noexcept;
  bool WakeUpOneDeepSleepingWorker() noexcept;
  FiberEntity* GetOrInstantiateFiber(RunnableEntity* entity) noexcept;
  bool QueueRunnableEntity(RunnableEntity* entity, bool sg_local, bool wait = false) noexcept;
  bool Push(RunnableEntity* entity, bool sg_local, bool wait) noexcept;
  void PostResume(FiberEntity* fiber) noexcept;

 private:
  // This class guarantees no wake-up loss by keeping a "wake-up count". If a wake
  // operation is made before a wait, the subsequent wait is immediately
  // satisfied without actual going to sleep.
  class alignas(hardware_destructive_interference_size) WaitSlot {
   public:
    void Wake() noexcept;

    void Wait() noexcept;

    void PersistentWake() noexcept;

   private:
    // `futex` requires this.
    static_assert(sizeof(std::atomic<int>) == sizeof(int));

    std::atomic<int> wakeup_count_{1};
  };

  struct Victim {
    SchedulingGroup* sg;
    std::uint64_t steal_every_n;
    std::uint64_t next_steal;

    // `std::priority_queue` orders elements descendingly.
    bool operator<(const Victim& v) const { return next_steal > v.next_steal; }
  };

  static constexpr auto kUninitializedWorkerIndex = std::numeric_limits<std::size_t>::max();
  static thread_local std::size_t worker_index_;

  SchedulingGroup* scheduling_group_;
  std::size_t group_size_;

  std::atomic<bool> stopped_{false};

  // Exposes internal state.
  // DelayedInit<tvar::PassiveStatus<std::string>> spinning_workers_var_, sleeping_workers_var_;

  // Ready fibers are put here.
  RunQueue run_queue_;

  // Fiber workers sleep on this.
  std::unique_ptr<WaitSlot[]> wait_slots_{nullptr};

  // Bit mask.
  //
  // We carefully chose to use 1 to represent "spinning" and "sleeping", instead
  // of "running" and "awake", here. This way if the number of workers is
  // smaller than 64, those not-used bit is treated as if they represent running
  // workers, and don't need special treat.
  alignas(hardware_destructive_interference_size) std::atomic<std::uint64_t> spinning_workers_{0};
  alignas(hardware_destructive_interference_size) std::atomic<std::uint64_t> sleeping_workers_{0};

  // Set if the last spinner successfully grabbed a fiber to run. In this case
  // we're likely under load, so it set this flag for other spinners (if there
  // is one) to wake more workers up (and hopefully to get a fiber or spin).
  alignas(hardware_destructive_interference_size) std::atomic<bool> pending_spinner_wakeup_{false};

  std::unique_ptr<std::uint64_t[]> steal_vec_clock_;
  std::unique_ptr<std::priority_queue<Victim>[]> victims_;
};

}  // namespace trpc::fiber::detail::v1
