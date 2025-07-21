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

#include <atomic>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "trpc/runtime/iomodel/reactor/default/timer_queue.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"
#include "trpc/runtime/threadmodel/separate/separate_scheduling.h"
#include "trpc/util/queue/bounded_mpmc_queue.h"
#include "trpc/util/queue/unbounded_spmc_queue.h"
#include "trpc/util/thread/predicate_notifier.h"

namespace trpc::separate {

/// @brief Implementation of non-coroutine handle task scheduling in io/handle separate thread model, with scheduling
///        algorithm inspired by the implementation of taskflow runtime. Related information can be found at:
///        https://github.com/taskflow/taskflow/blob/master/taskflow/core/executor.hpp.
///        Applicable scenarios: CPU parallel computing scenario, such as DAG-type tasks.
///        Implementation principle and advantages:
///        1. The task queue consists of a global (mpmc) and local (spmc) queue, supporting task stealing between global
///        and local queues.
///        2. When executing parallel tasks, the worker thread can add tasks to its local queue without the need for
///        notification, resulting in no system call overhead.
///        3. Inter-thread notification uses the native notifier of taskflow, resulting in lower notification overhead.
/// @note It does not support delivering tasks to specified thread for execution.

class StealScheduling final : public SeparateScheduling {
 public:
  // @brief configuration options of scheduling
  struct Options {
    /// @brief name of the related thread model instance
    std::string group_name;

    /// @brief number of worker thread
    uint16_t worker_thread_num;

    /// @brief size of the global queue
    uint32_t global_queue_size = 50000;
  };

  explicit StealScheduling(Options&& options);

  ~StealScheduling() override = default;

  std::string_view Name() const override { return kStealScheduling; }

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
  bool Push(MsgTask* task) noexcept;
  uint32_t Size(std::size_t worker_index);

  void HandleTimerTask(std::size_t worker_index) noexcept;
  uint64_t CreateTimer(std::size_t worker_index, TimerTask* timer_task) noexcept;
  void PauseTimer(std::size_t worker_index, uint64_t timer_id) noexcept;
  void ResumeTimer(std::size_t worker_index, uint64_t timer_id) noexcept;
  void CancelTimer(std::size_t worker_index, uint64_t timer_id) noexcept;

  uint32_t GetWaitTimeout(std::size_t worker_index) noexcept;

  bool WaitForTask(MsgTask*& task, size_t worker_index) noexcept;
  void ExploreTask(MsgTask*& t) noexcept;

 private:
  Options options_;

  std::atomic<bool> stopped_{false};

  std::vector<std::unique_ptr<TimerQueue>> timer_queues_;

  PredicateNotifier notifier_;
  std::vector<PredicateNotifier::Waiter*> waiters_;

  BoundedMPMCQueue<MsgTask*> global_task_queue_;
  std::unique_ptr<UnboundedSPMCQueue<MsgTask*>[]> local_task_queues_;

  std::atomic<size_t> num_actives_{0};
  std::atomic<size_t> num_thieves_{0};
  std::vector<size_t> vtm_;
  std::uniform_int_distribution<size_t> rdvtm_;
  std::default_random_engine rdgen_{std::random_device{}()};  // NOLINT
};

}  // namespace trpc::separate
