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

#include "trpc/runtime/threadmodel/separate/non_steal/non_steal_scheduling.h"

#include "trpc/runtime/common/heartbeat/heartbeat_info.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool.h"
#include "trpc/util/time.h"

namespace trpc::separate {

NonStealScheduling::NonStealScheduling(Options&& options) : options_(std::move(options)) {
  TRPC_ASSERT(options_.worker_thread_num > 0);
  TRPC_ASSERT(options_.global_queue_size > 0);
  TRPC_ASSERT(options_.local_queue_size > 0);

  TRPC_ASSERT(global_task_queue_.Init(options_.global_queue_size) == true);

  local_task_queues_ = std::make_unique<BoundedMPMCQueue<MsgTask*>[]>(options_.worker_thread_num);
  for (size_t i = 0; i < options_.worker_thread_num; ++i) {
    TRPC_ASSERT(local_task_queues_[i].Init(options_.local_queue_size) == true);
  }

  for (size_t i = 0; i < options_.worker_thread_num; ++i) {
    timer_queues_.push_back(std::make_unique<TimerQueue>());
  }
}

void NonStealScheduling::Enter(int32_t current_worker_thread_id) noexcept {
  TRPC_ASSERT(current_worker_thread_id < options_.worker_thread_num);

  SetCurrentScheduling(this);
  SetCurrentWorkerIndex(current_worker_thread_id);
}

void NonStealScheduling::Schedule() noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();

  while (!stopped_.load(std::memory_order_relaxed)) {
    if (Size(worker_index) == 0) {
      uint32_t timeout = GetWaitTimeout(worker_index);
      if (timeout > 0) {
        Wait(timeout);
      }
    }

    HandleMsgTask(worker_index);

    HandleTimerTask(worker_index);
  }
}

void NonStealScheduling::ExecuteTask(std::size_t worker_index) noexcept {
  HandleMsgTask(worker_index);

  HandleTimerTask(worker_index);
}

void NonStealScheduling::HandleMsgTask(std::size_t worker_index) noexcept {
  uint32_t execute_count = 100;
  while (execute_count > 0) {
    // report its own heartbeat information before each task execution.
    HeartBeat(Size(worker_index));

    MsgTask* task = Pop(worker_index);
    if (task) {
      task->handler();

      trpc::object_pool::Delete<MsgTask>(task);
    } else {
      break;
    }

    --execute_count;
  }
}

void NonStealScheduling::HandleTimerTask(std::size_t worker_index) noexcept {
  timer_queues_[worker_index]->RunExpiredTimers(trpc::time::GetMilliSeconds());
}

uint32_t NonStealScheduling::GetWaitTimeout(std::size_t worker_index) noexcept {
  uint64_t current_ms = trpc::time::GetMilliSeconds();
  uint64_t timer_task_expiration = timer_queues_[worker_index]->NextExpiration();
  if (timer_task_expiration > current_ms) {
    return (timer_task_expiration - current_ms) > 10 ? 10 : (timer_task_expiration - current_ms);
  }

  return 0;
}

void NonStealScheduling::Leave() noexcept {
  SetCurrentScheduling(nullptr);
  SetCurrentWorkerIndex(-1);
}

bool NonStealScheduling::SubmitIoTask(MsgTask* io_task) noexcept { return false; }

bool NonStealScheduling::SubmitHandleTask(MsgTask* handle_task) noexcept {
  bool ret = Push(handle_task);
  if (ret) {
    Notify();
  } else {
    object_pool::Delete<MsgTask>(handle_task);
  }

  return ret;
}

uint64_t NonStealScheduling::AddTimer(TimerTask* timer_task) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    return timer_queues_[worker_index]->AddTimer(timer_task);
  }

  return kInvalidTimerId;
}

void NonStealScheduling::PauseTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->PauseTimer(timer_id);
  }
}

void NonStealScheduling::ResumeTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->ResumeTimer(timer_id);
  }
}

void NonStealScheduling::CancelTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->CancelTimer(timer_id);
  }
}

void NonStealScheduling::DetachTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->DetachTimer(timer_id);
  }
}

void NonStealScheduling::Stop() noexcept {
  stopped_.store(true, std::memory_order_relaxed);

  Notify();
}

uint32_t NonStealScheduling::Size(std::size_t worker_index) const {
  return global_task_queue_.Size() + local_task_queues_[worker_index].Size();
}

uint32_t NonStealScheduling::Capacity(std::size_t worker_index) const {
  return global_task_queue_.Capacity() + local_task_queues_[worker_index].Capacity();
}

bool NonStealScheduling::Push(MsgTask* task) noexcept {
  if (task->dst_thread_key < 0) {
    switch (task->task_type) {
      case trpc::runtime::kParallelTask:
        return global_task_queue_.Push(task);

      default:
        std::size_t worker_index = GetCurrentWorkerIndex();
        if (worker_index == static_cast<std::size_t>(-1)) {
          return global_task_queue_.Push(task);
        } else {
          return local_task_queues_[worker_index].Push(task);
        }
    }
  } else {
    std::size_t worker_index = task->dst_thread_key % options_.worker_thread_num;
    return local_task_queues_[worker_index].Push(task);
  }

  return false;
}

MsgTask* NonStealScheduling::Pop(std::size_t worker_index) noexcept {
  MsgTask* msg_task{nullptr};
  if (local_task_queues_[worker_index].Size() > 0) {
    if (local_task_queues_[worker_index].Pop(msg_task)) {
      return msg_task;
    }
  }

  if (global_task_queue_.Size() > 0) {
    if (global_task_queue_.Pop(msg_task)) {
      return msg_task;
    }
  }

  return nullptr;
}

void NonStealScheduling::Notify() noexcept {
  ThreadLock::LockT lock(notifier_);
  notifier_.NotifyAll();
}

void NonStealScheduling::Wait(uint32_t timeout_ms) noexcept {
  ThreadLock::LockT lock(notifier_);
  notifier_.TimedWait(timeout_ms);
}

void NonStealScheduling::Destroy() noexcept {
  local_task_queues_.reset();
  timer_queues_.clear();
}

}  // namespace trpc::separate
