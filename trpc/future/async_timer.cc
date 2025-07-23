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

#include "trpc/future/async_timer.h"

#include <utility>

#include "trpc/runtime/runtime.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/check.h"
#include "trpc/util/chrono/time.h"

namespace trpc {

namespace iotimer {

uint64_t Create(uint64_t delay, uint64_t interval, Function<void()>&& task) {
  auto reactor = runtime::GetThreadLocalReactor();
  if (!reactor) {
    TRPC_FMT_ERROR("current thread has no reactor");
    return kInvalidTimerId;
  }

  return reactor->AddTimerAfter(delay, interval, std::move(task));
}

bool Detach(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return false;
  }

  auto reactor = runtime::GetThreadLocalReactor();
  if (!reactor) {
    TRPC_FMT_ERROR("current thread has no reactor");
    return false;
  }

  reactor->DetachTimer(timer_id);
  return true;
}

bool Cancel(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return false;
  }

  auto reactor = runtime::GetThreadLocalReactor();
  if (!reactor) {
    TRPC_FMT_ERROR("current thread has no reactor");
    return false;
  }

  reactor->CancelTimer(timer_id);
  return true;
}

bool Pause(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return false;
  }

  auto reactor = runtime::GetThreadLocalReactor();
  if (!reactor) {
    TRPC_FMT_ERROR("current thread has no reactor");
    return false;
  }

  reactor->PauseTimer(timer_id);
  return true;
}

bool Resume(uint64_t timer_id) {
  if (TRPC_UNLIKELY(timer_id == kInvalidTimerId)) {
    return false;
  }

  auto reactor = runtime::GetThreadLocalReactor();
  if (!reactor) {
    TRPC_FMT_ERROR("current thread has no reactor");
    return false;
  }

  reactor->ResumeTimer(timer_id);
  return true;
}

}  // namespace iotimer

namespace {

uint64_t CreateTimerInMerge(WorkerThread* current_thread, TimerTask* timer_task) {
  Reactor* reactor = static_cast<MergeWorkerThread*>(current_thread)->GetReactor();
  if (!reactor) {
    return kInvalidTimerId;
  }

  return reactor->AddTimer(timer_task);
}

uint64_t CreateTimerInSeparate(WorkerThread* current_thread, TimerTask* timer_task) {
  SeparateScheduling* scheduling = static_cast<HandleWorkerThread*>(current_thread)->GetSeparateScheduling();
  if (!scheduling) {
    return kInvalidTimerId;
  }

  return scheduling->AddTimer(timer_task);
}

uint64_t CreateTimer(TimerTask* timer_task) {
  WorkerThread* current_thread = WorkerThread::GetCurrentWorkerThread();
  if (!current_thread) {
    return kInvalidTimerId;
  }

  if (current_thread->Role() != kHandle) {
    return CreateTimerInMerge(current_thread, timer_task);
  }

  return CreateTimerInSeparate(current_thread, timer_task);
}

void CancelTimerInMerge(WorkerThread* current_thread, uint64_t timer_id) {
  Reactor* reactor = static_cast<MergeWorkerThread*>(current_thread)->GetReactor();
  if (!reactor) {
    return;
  }

  reactor->CancelTimer(timer_id);
}

void CancelTimerInSeparate(WorkerThread* current_thread, uint64_t timer_id) {
  SeparateScheduling* scheduling = static_cast<HandleWorkerThread*>(current_thread)->GetSeparateScheduling();
  if (!scheduling) {
    return;
  }

  scheduling->CancelTimer(timer_id);
}

void CancelTimer(uint64_t timer_id) {
  WorkerThread* current_thread = WorkerThread::GetCurrentWorkerThread();
  if (!current_thread) {
    return;
  }

  if (current_thread->Role() != kHandle) {
    CancelTimerInMerge(current_thread, timer_id);
  } else {
    CancelTimerInSeparate(current_thread, timer_id);
  }
}

void DetachTimerInMerge(WorkerThread* current_thread, uint64_t timer_id) {
  Reactor* reactor = static_cast<MergeWorkerThread*>(current_thread)->GetReactor();
  if (!reactor) {
    return;
  }

  reactor->DetachTimer(timer_id);
}

void DetachTimerInSeparate(WorkerThread* current_thread, uint64_t timer_id) {
  SeparateScheduling* scheduling = static_cast<HandleWorkerThread*>(current_thread)->GetSeparateScheduling();
  if (!scheduling) {
    return;
  }

  scheduling->DetachTimer(timer_id);
}

void DetachTimer(uint64_t timer_id) {
  WorkerThread* current_thread = WorkerThread::GetCurrentWorkerThread();
  if (!current_thread) {
    return;
  }

  if (current_thread->Role() != kHandle) {
    DetachTimerInMerge(current_thread, timer_id);
  } else {
    DetachTimerInSeparate(current_thread, timer_id);
  }
}

}  // namespace

AsyncTimer::~AsyncTimer() {
  if (auto_cancel_) {
    AutoCancel();
  } else {
    if (timer_id_ != kInvalidTimerId) {
      DetachTimer(timer_id_);
      timer_id_ = kInvalidTimerId;
    }
  }
}

Future<> AsyncTimer::After(uint64_t ms) {
  if (timer_id_ == kInvalidTimerId) {
    promise_ = std::make_shared<Promise<>>();

    auto timer_task = object_pool::MakeLwShared<TimerTask>();
    timer_task->create = true;
    timer_task->expiration = time::GetMilliSeconds() + ms;
    timer_task->interval = 0;
    timer_task->executor = [pr = promise_]() {
      if (!pr->IsFailed()) {
        pr->SetValue();
      }
    };

    uint64_t result = CreateTimer(timer_task.Leak());
    if (result != kInvalidTimerId) {
      timer_id_ = result;

      return promise_->GetFuture();
    }

    promise_ = nullptr;
    return MakeExceptionFuture<>(CreateFailed());
  }

  return MakeExceptionFuture<>(Unavailable());
}

bool AsyncTimer::After(uint64_t delay, Function<void(void)>&& executor, int interval) {
  if (timer_id_ == kInvalidTimerId) {
    auto timer_task = object_pool::MakeLwShared<TimerTask>();
    timer_task->create = true;
    timer_task->expiration = time::GetMilliSeconds() + delay;
    timer_task->interval = (interval > 0 ? interval : 0);
    timer_task->executor = std::move(executor);

    uint64_t result = CreateTimer(timer_task.Leak());
    if (result != kInvalidTimerId) {
      timer_id_ = result;
      return true;
    }

    return false;
  }

  return false;
}

void AsyncTimer::Cancel() {
  if (timer_id_ == kInvalidTimerId) {
    return;
  }

  if (promise_ && !promise_->IsReady()) {
    promise_->SetException(Cancelled());
  }

  CancelTimer(timer_id_);

  timer_id_ = kInvalidTimerId;
  auto_cancel_ = true;
  promise_ = nullptr;
}

void AsyncTimer::AutoCancel() {
  Cancel();
}

void AsyncTimer::Detach() {
  if (timer_id_ == kInvalidTimerId) {
    return;
  }

  DetachTimer(timer_id_);

  timer_id_ = kInvalidTimerId;
  auto_cancel_ = true;
  promise_ = nullptr;
}

}  // namespace trpc
