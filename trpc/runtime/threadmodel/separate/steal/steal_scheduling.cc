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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. taskflow
// Copyright (c) 2018-2023 Dr. Tsung-Wei Huang
// taskflow is licensed under the MIT License.
//

#include "trpc/runtime/threadmodel/separate/steal/steal_scheduling.h"

#include "trpc/runtime/common/heartbeat/heartbeat_info.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool.h"
#include "trpc/util/time.h"

namespace trpc::separate {

StealScheduling::StealScheduling(Options&& options)
    : options_(std::move(options)), notifier_{options_.worker_thread_num}, rdvtm_(0, options_.worker_thread_num - 1) {
  TRPC_ASSERT(options_.worker_thread_num > 0);
  TRPC_ASSERT(options_.global_queue_size > 0);

  TRPC_ASSERT(global_task_queue_.Init(options_.global_queue_size) == true);

  local_task_queues_ = std::make_unique<UnboundedSPMCQueue<MsgTask*>[]>(options_.worker_thread_num);
  vtm_.assign(options_.worker_thread_num, 0);

  waiters_.resize(options_.worker_thread_num);
  for (int i = 0; i < options_.worker_thread_num; i++) {
    waiters_[i] = &notifier_.GetWaiter(i);
  }

  for (size_t i = 0; i < options_.worker_thread_num; ++i) {
    timer_queues_.push_back(std::make_unique<TimerQueue>());
  }
}

void StealScheduling::Enter(int32_t current_worker_thread_id) noexcept {
  TRPC_ASSERT(current_worker_thread_id < options_.worker_thread_num);

  SetCurrentScheduling(this);
  SetCurrentWorkerIndex(current_worker_thread_id);
  TRPC_ASSERT(current_worker_thread_id >= 0 && current_worker_thread_id < options_.worker_thread_num);
  vtm_[current_worker_thread_id] = current_worker_thread_id;
}

// The following source codes are from taskflow.
// Copied and modified from
// https://github.com/taskflow/taskflow/blob/master/taskflow/core/executor.hpp

void StealScheduling::Schedule() noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  MsgTask* task = nullptr;
  while (1) {
    if (task) {
      if (num_actives_.fetch_add(1) == 0 && num_thieves_ == 0) {
        notifier_.Notify(false);
      }

      while (task) {
        task->handler();
        trpc::object_pool::Delete<MsgTask>(task);
        task = local_task_queues_[worker_index].Pop();

        HandleTimerTask(worker_index);

        // report task queue size
        HeartBeat(Size(worker_index));
      }

      --num_actives_;
    } else {
      HandleTimerTask(worker_index);

      HeartBeat(Size(worker_index));
    }

    if (WaitForTask(task, worker_index) == false) {
      break;
    }
  }
}

bool StealScheduling::WaitForTask(MsgTask*& t, size_t worker_index) noexcept {
  ++num_thieves_;

  ExploreTask(t);

  if (t) {
    if (num_thieves_.fetch_sub(1) == 1) {
      notifier_.Notify(false);
    }
    return true;
  }

  notifier_.PrepareWait(waiters_[worker_index]);

  if (global_task_queue_.Size()) {
    notifier_.CancelWait(waiters_[worker_index]);

    MsgTask* msg_task{nullptr};
    if (global_task_queue_.Pop(msg_task)) {
      if (num_thieves_.fetch_sub(1) == 1) {
        notifier_.Notify(false);
      }
      t = msg_task;
      return true;
    } else {
      vtm_[worker_index] = worker_index;
      --num_thieves_;
      return true;
    }
  }

  if (stopped_.load(std::memory_order_relaxed)) {
    notifier_.CancelWait(waiters_[worker_index]);
    notifier_.Notify(true);
    --num_thieves_;
    return false;
  }

  if (num_thieves_.fetch_sub(1) == 1) {
    if (num_actives_) {
      notifier_.CancelWait(waiters_[worker_index]);
      return true;
    }
    // check all queues again
    for (int i = 0; i < options_.worker_thread_num; i++) {
      auto& wsq = local_task_queues_[i];
      if (!wsq.Empty()) {
        vtm_[worker_index] = i;
        notifier_.CancelWait(waiters_[worker_index]);
        return true;
      }
    }
  }

  // Now I really need to relinguish my self to others
  notifier_.CommitWait(waiters_[worker_index]);

  return true;
}

void StealScheduling::ExploreTask(MsgTask*& t) noexcept {
  size_t num_steals = 0;
  size_t num_yields = 0;
  size_t max_steals = ((options_.worker_thread_num + 1) << 1);
  size_t worker_index = GetCurrentWorkerIndex();

  do {
    if ((worker_index == vtm_[worker_index])) {
      global_task_queue_.Pop(t);
    } else {
      t = local_task_queues_[vtm_[worker_index]].Steal();
    }

    if (t) {
      break;
    }

    if (num_steals++ > max_steals) {
      std::this_thread::yield();
      if (num_yields++ > 100) {
        break;
      }
    }

    vtm_[worker_index] = rdvtm_(rdgen_);
  } while (!stopped_.load(std::memory_order_relaxed));
}

// End of source codes that are from taskflow.

void StealScheduling::ExecuteTask(std::size_t worker_index) noexcept {
  if (auto t = local_task_queues_[worker_index].Pop(); t) {
    t->handler();
    trpc::object_pool::Delete<MsgTask>(t);
  } else {
    if ((worker_index == vtm_[worker_index])) {
      global_task_queue_.Pop(t);
    } else {
      t = local_task_queues_[vtm_[worker_index]].Steal();
    }

    if (t) {
      t->handler();
      trpc::object_pool::Delete<MsgTask>(t);
    } else {
      vtm_[worker_index] = rdvtm_(rdgen_);
    }
  }

  HandleTimerTask(worker_index);

  HeartBeat(Size(worker_index));
}

void StealScheduling::HandleTimerTask(std::size_t worker_index) noexcept {
  timer_queues_[worker_index]->RunExpiredTimers(trpc::time::GetMilliSeconds());
}

uint32_t StealScheduling::GetWaitTimeout(std::size_t worker_index) noexcept {
  uint64_t current_ms = trpc::time::GetMilliSeconds();
  uint64_t timer_task_expiration = timer_queues_[worker_index]->NextExpiration();
  if (timer_task_expiration > current_ms) {
    return (timer_task_expiration - current_ms) > 10 ? 10 : (timer_task_expiration - current_ms);
  }

  return 0;
}

void StealScheduling::Leave() noexcept {
  SetCurrentScheduling(nullptr);
  SetCurrentWorkerIndex(-1);
}

bool StealScheduling::SubmitIoTask(MsgTask* io_task) noexcept { return false; }

bool StealScheduling::SubmitHandleTask(MsgTask* handle_task) noexcept {
  bool ret = Push(handle_task);
  if (TRPC_UNLIKELY(!ret)) {
    object_pool::Delete<MsgTask>(handle_task);
  }

  return ret;
}

uint64_t StealScheduling::AddTimer(TimerTask* timer_task) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    return timer_queues_[worker_index]->AddTimer(timer_task);
  }

  return kInvalidTimerId;
}

void StealScheduling::PauseTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->PauseTimer(timer_id);
  }
}

void StealScheduling::ResumeTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->ResumeTimer(timer_id);
  }
}

void StealScheduling::CancelTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->CancelTimer(timer_id);
  }
}

void StealScheduling::DetachTimer(uint64_t timer_id) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    TRPC_ASSERT(worker_index < options_.worker_thread_num);
    timer_queues_[worker_index]->DetachTimer(timer_id);
  }
}

void StealScheduling::Stop() noexcept {
  stopped_.store(true, std::memory_order_relaxed);
  notifier_.Notify(true);
}

uint32_t StealScheduling::Size(std::size_t worker_index) {
  return local_task_queues_[worker_index].Size() + global_task_queue_.Size();
}

bool StealScheduling::Push(MsgTask* task) noexcept {
  std::size_t worker_index = GetCurrentWorkerIndex();
  if (worker_index != static_cast<std::size_t>(-1)) {
    local_task_queues_[worker_index].Push(task);
  } else {
    bool ret = global_task_queue_.Push(task);
    if (ret) {
      notifier_.Notify(false);
    }
    return ret;
  }
  return true;
}

void StealScheduling::Destroy() noexcept {
  local_task_queues_.reset();
  timer_queues_.clear();
}

}  // namespace trpc::separate
