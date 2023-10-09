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

#include "trpc/runtime/iomodel/reactor/default/timer_queue.h"

#include "trpc/util/check.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

TimerQueue::TimerQueue()
    : timer_task_queue_(65536) {
}

uint64_t TimerQueue::AddTimer(TimerTask* timer_task) {
  uint64_t timer_id = reinterpret_cast<std::uint64_t>(timer_task);

  TimerTaskPtr ptr(object_pool::lw_shared_ref_ptr, timer_task);
  timer_task->owner = this;
  ptr->timer_id = timer_id;

  TRPC_DCHECK_EQ(ptr->UseCount(), std::uint32_t(2));

  timer_task_queue_.push(std::move(ptr));

  return timer_id;
}

uint64_t TimerQueue::AddTimerAt(uint64_t expiration, uint64_t interval, Reactor::TimerExecutor&& executor) {
  auto timer_task = object_pool::MakeLwShared<TimerTask>();
  timer_task->owner = this;
  timer_task->cancel = false;
  timer_task->pause = false;
  timer_task->expiration = expiration;
  timer_task->interval = interval;
  timer_task->executor = std::move(executor);

  uint64_t timer_id = reinterpret_cast<std::uint64_t>(timer_task.Leak());

  TimerTaskPtr ptr(object_pool::lw_shared_ref_ptr, reinterpret_cast<TimerTask*>(timer_id));
  ptr->timer_id = timer_id;

  TRPC_DCHECK_EQ(ptr->UseCount(), std::uint32_t(2));

  timer_task_queue_.push(std::move(ptr));

  return timer_id;
}

uint64_t TimerQueue::AddTimerAfter(uint64_t delay, uint64_t interval, Reactor::TimerExecutor&& executor) {
  uint64_t expiration = trpc::time::GetMilliSeconds() + delay;

  return AddTimerAt(expiration, interval, std::move(executor));
}

void TimerQueue::PauseTimer(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return;
  }

  TimerTaskPtr ptr(object_pool::lw_shared_ref_ptr, reinterpret_cast<TimerTask*>(timer_id));
  TRPC_CHECK_EQ(ptr->owner, this,
                "The timer you're trying to pause does not belong to this timer queue.");

  if (!ptr->pause) {
    ptr->pause = true;
  }
}

void TimerQueue::ResumeTimer(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return;
  }

  TimerTaskPtr ptr(object_pool::lw_shared_ref_ptr, reinterpret_cast<TimerTask*>(timer_id));
  TRPC_CHECK_EQ(ptr->owner, this,
                "The timer you're trying to resume does not belong to this timer queue.");

  if (ptr->pause) {
    ptr->pause = false;
  }
}

void TimerQueue::CancelTimer(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return;
  }

  TimerTaskPtr ptr(object_pool::lw_shared_adopt_ptr, reinterpret_cast<TimerTask*>(timer_id));
  TRPC_CHECK_EQ(ptr->owner, this,
                "The timer you're trying to cancel does not belong to this timer queue.");
  TRPC_CHECK_GE(ptr->UseCount(), std::uint32_t(1));

  if (!ptr->cancel) {
    ptr->cancel = true;
  }
}

void TimerQueue::DetachTimer(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return;
  }

  TimerTaskPtr ptr(object_pool::lw_shared_adopt_ptr, reinterpret_cast<TimerTask*>(timer_id));
  TRPC_CHECK_EQ(ptr->owner, this,
                "The timer you're trying to detach does not belong to this timer queue.");
}

void TimerQueue::RunExpiredTimers(uint64_t now_ms) {
  if (timer_task_queue_.empty()) {
    return;
  }

  while (!timer_task_queue_.empty()) {
    auto timer_task = timer_task_queue_.top();
    if (timer_task->cancel) {
      timer_task_queue_.pop();
      continue;
    }

    if (timer_task->expiration > now_ms) {
      break;
    }

    if (!timer_task->pause) {
      timer_task->executor();
    }

    if (timer_task->interval != 0) {
      timer_task->expiration += timer_task->interval;
      timer_task_queue_.push(std::move(timer_task));
    }

    timer_task_queue_.pop();
  }
}

uint64_t TimerQueue::NextExpiration() const {
  return !timer_task_queue_.empty() ? timer_task_queue_.top()->expiration : kInvalidTimerId;
}

uint32_t TimerQueue::Size() const { return timer_task_queue_.size(); }

}  // namespace trpc
