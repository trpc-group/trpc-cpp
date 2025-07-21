//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include "trpc/runtime/iomodel/reactor/default/timer_queue.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"
#include "trpc/runtime/threadmodel/separate/separate_scheduling.h"
#include "trpc/util/queue/bounded_mpmc_queue.h"
#include "trpc/util/queue/bounded_mpsc_queue.h"
#include "trpc/util/thread/thread_monitor.h"

namespace trpc::separate {

/// @brief Implementation of non-coroutine handle task scheduling in io/handle separate thread model
class NonStealScheduling final : public SeparateScheduling {
 public:
  struct Options {
    /// @brief name of the related thread model instance
    std::string group_name;

    /// @brief number of worker thread
    uint16_t worker_thread_num;

    /// @brief size of the global queue
    uint32_t global_queue_size = 50000;

    /// @brief size of the current thread's local queue
    uint32_t local_queue_size = 50000;
  };

  explicit NonStealScheduling(Options&& options);

  ~NonStealScheduling() override = default;

  std::string_view Name() const override { return kNonStealScheduling; }

  void Enter(int32_t current_worker_thread_id) noexcept override;

  void Schedule() noexcept override;

  void ExecuteTask(std::size_t worker_index) noexcept override;

  void Leave() noexcept override;

  bool SubmitIoTask(MsgTask* io_task) noexcept override;

  bool SubmitHandleTask(MsgTask* handle_task) noexcept override;

  uint64_t AddTimer(TimerTask* timer_task) noexcept override;
  void PauseTimer(uint64_t timer_id) noexcept override;
  void ResumeTimer(uint64_t timer_id) noexcept override;
  void CancelTimer(uint64_t timer_id) noexcept override;
  void DetachTimer(uint64_t timer_id) noexcept override;

  void Stop() noexcept override;

  void Destroy() noexcept override;

 private:
  void HandleMsgTask(std::size_t worker_index) noexcept;
  bool Push(MsgTask* task) noexcept;
  MsgTask* Pop(std::size_t worker_index) noexcept;
  uint32_t Size(std::size_t worker_index) const;
  uint32_t Capacity(std::size_t worker_index) const;

  void HandleTimerTask(std::size_t worker_index) noexcept;
  uint64_t CreateTimer(std::size_t worker_index, TimerTask* timer_task) noexcept;
  void PauseTimer(std::size_t worker_index, uint64_t timer_id) noexcept;
  void ResumeTimer(std::size_t worker_index, uint64_t timer_id) noexcept;
  void CancelTimer(std::size_t worker_index, uint64_t timer_id) noexcept;

  void Notify() noexcept;
  uint32_t GetWaitTimeout(std::size_t worker_index) noexcept;
  void Wait(uint32_t timeout_ms) noexcept;

 private:
  Options options_;

  std::atomic<bool> stopped_{false};

  ThreadLock notifier_;

  BoundedMPMCQueue<MsgTask*> global_task_queue_;

  std::unique_ptr<BoundedMPMCQueue<MsgTask*>[]> local_task_queues_;

  std::vector<std::unique_ptr<TimerQueue>> timer_queues_;
};

}  // namespace trpc::separate
