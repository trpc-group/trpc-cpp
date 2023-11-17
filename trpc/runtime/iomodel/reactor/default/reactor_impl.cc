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

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"

#include <utility>

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
#include "trpc/util/async_io/async_io.h"
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
#include "trpc/runtime/common/heartbeat/heartbeat_info.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

ReactorImpl::ReactorImpl(const Options& options)
    : Reactor(),
      options_(options),
      task_notifier_(this),
      stop_notifier_(this) {
  poller_.SetWaitCallback([this](int) { is_polling_ = true; });

  if (options_.max_task_queue_size == 0) {
    options_.max_task_queue_size = 50000;
  }

  for (auto& queue : task_queues_) {
    auto& q = queue.q;
    TRPC_ASSERT(q.Init(options_.max_task_queue_size));
  }

  if (options_.enable_async_io) {
#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
    AsyncIO::Options async_io_options;
    async_io_options.reactor = this;
    async_io_options.entries = options_.io_uring_entries;
    async_io_options.flags = options_.io_uring_flags;

    async_io_ = std::make_unique<AsyncIO>(async_io_options);
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
  }
}

bool ReactorImpl::Initialize() {
  stop_notifier_.EnableNotify();
  task_notifier_.EnableNotify();

  if (options_.enable_async_io) {
#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
    async_io_->EnableNotify();
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
  }

  return true;
}

void ReactorImpl::Stop() {
  stopped_.store(true, std::memory_order_relaxed);

  stop_notifier_.WakeUp();
}

void ReactorImpl::Join() {}

void ReactorImpl::Destroy() {
  task_notifier_.DisableNotify();
  stop_notifier_.DisableNotify();

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
  if (async_io_) {
    async_io_->DisableNotify();
  }
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
}

void ReactorImpl::ExecuteTask() {
  HeartBeat(GetTaskSize());

  HandleTask(false);

  TrySleep(true);
}

uint64_t ReactorImpl::GetEpollWaitTimeout() {
  uint64_t timeout = timer_queue_.NextExpiration() - trpc::time::GetMilliSeconds();
  return timeout < Poller::kPollerTimeout ? timeout : Poller::kPollerTimeout;
}

void ReactorImpl::Run() {
  SetCurrentTlsReactor(this);

  while (!stopped_.load(std::memory_order_relaxed)) {
    HeartBeat(GetTaskSize());

    bool left = HandleTask(false);
    TrySleep(left);

    timer_queue_.RunExpiredTimers(trpc::time::GetMilliSeconds());
  }

  HandleTask(true);

  SetCurrentTlsReactor(nullptr);
}

void ReactorImpl::Update(EventHandler* event_handler) {
  poller_.UpdateEvent(event_handler);
}

bool ReactorImpl::SubmitTask(Task&& task, Priority priority) {
  auto& q = task_queues_[static_cast<int>(priority)].q;
  if (!q.Push(std::move(task))) {
    TRPC_FMT_WARN_IF(TRPC_WITHIN_N(1000), "Push task into reactor #{} failed.", options_.id);
    return false;
  }

  // Wake up reactor if needed
  if (!is_polling_) {
    task_notifier_.WakeUp();
  }

  return true;
}

bool ReactorImpl::SubmitTask2(Task&& task, Priority priority) {
  if (GetCurrentTlsReactor() == this) {
    task();
    return true;
  }

  auto& q = task_queues_[static_cast<int>(priority)].q;
  if (!q.Push(std::move(task))) {
    TRPC_FMT_WARN_IF(TRPC_WITHIN_N(1000), "Enqueue task into reactor #{} failed.", options_.id);
    return false;
  }
  // Wake up reactor if needed
  if (!is_polling_) {
    task_notifier_.WakeUp();
  }

  return true;
}

bool ReactorImpl::SubmitInnerTask(Task&& task) {
  {
    std::scoped_lock _(mutex_);
    inner_task_queue_.push_back(std::move(task));
  }

  // wake up reactor if needed
  if (!is_polling_) {
    task_notifier_.WakeUp();
  }

  return true;
}

bool ReactorImpl::CheckTask() {
  {
    std::scoped_lock _(mutex_);
    if (inner_task_queue_.size() > 0) {
      return true;
    }
  }

  for (auto& queue : task_queues_) {
    auto& q = queue.q;
    if (q.Size() > 0) {
      return true;
    }
  }
  return false;
}

bool ReactorImpl::HandleTask(bool clear) {
  std::list<Task> task_queue;
  {
    std::scoped_lock lk(mutex_);
    task_queue.swap(inner_task_queue_);
  }

  while (!task_queue.empty()) {
    task_queue.front()();
    task_queue.pop_front();
  }

  int idle = 0;
  int count = 0;
  while (true) {
    for (int i = static_cast<int>(Priority::kHigh); i >= static_cast<int>(Priority::kLowest); --i) {
      for (int j = static_cast<int>(Priority::kHigh); j >= i; --j) {
        auto& q = task_queues_[j].q;
        Task task;
        if (q.Pop(task)) {
          task();
          if (!clear && ++count >= kMaxTaskCountOnce) {
            return true;
          }
          idle = 0;
        } else {
          // Continuous empty polling
          if (++idle >= kContiguousEmptyPolling) {
            return false;
          }
        }
      }
    }
  }
}

bool ReactorImpl::TrySleep(bool ensure) {
  is_polling_ = false;
  if (!ensure && CheckTask()) {
    is_polling_ = true;
    return false;
  }

  int timeout_ms = GetEpollWaitTimeout();

  // From now on, if any new task is appended, the actual sleeping will be
  // interrupted quickly by the notifier.
  poller_.Dispatch(ensure ? 0 : timeout_ms);

  return true;
}

size_t ReactorImpl::GetTaskSize() {
  size_t task_size = 0;

  {
    std::scoped_lock _(mutex_);
    task_size = inner_task_queue_.size();
  }

  for (size_t i = 0; i < kPriorities; i++) {
    task_size += task_queues_[i].q.Size();
  }
  return task_size;
}

uint64_t ReactorImpl::AddTimer(TimerTask* timer_task) {
  return timer_queue_.AddTimer(timer_task);
}

uint64_t ReactorImpl::AddTimerAt(uint64_t expiration, uint64_t interval, TimerExecutor&& executor) {
  return timer_queue_.AddTimerAt(expiration, interval, std::move(executor));
}

uint64_t ReactorImpl::AddTimerAfter(uint64_t delay, uint64_t interval, TimerExecutor&& executor) {
  return timer_queue_.AddTimerAfter(delay, interval, std::move(executor));
}

void ReactorImpl::PauseTimer(uint64_t timer_id) { timer_queue_.PauseTimer(timer_id); }

void ReactorImpl::ResumeTimer(uint64_t timer_id) { timer_queue_.ResumeTimer(timer_id); }

void ReactorImpl::CancelTimer(uint64_t timer_id) { timer_queue_.CancelTimer(timer_id); }

void ReactorImpl::DetachTimer(uint64_t timer_id) { timer_queue_.DetachTimer(timer_id); }

}  // namespace trpc
