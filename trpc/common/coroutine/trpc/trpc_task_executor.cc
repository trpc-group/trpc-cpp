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

#include "trpc/common/coroutine/trpc/trpc_task_executor.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/runtime/runtime.h"
#include "trpc/util/thread/latch.h"
#include "trpc/util/log/logging.h"

namespace trpc::coroutine {

template <>
Status TrpcTaskExcutorImp<TaskExecutor<TrpcCoroutineTask>>::FiberExecTaskInFiberWorker() {
  size_t task_num = this->task_lists_.size();
  if (task_num > 1) {
    trpc::FiberLatch fiber_latch(task_num - 1);
    bool is_start_all_fiber_succ = true;
    for (size_t i = 1; i < task_num; ++i) {
      bool ret = trpc::StartFiberDetached([&fiber_latch, this, i] {
        this->task_lists_[i].Process();
        fiber_latch.CountDown();
      });

      if (TRPC_UNLIKELY(!ret)) {
        fiber_latch.CountDown();
        is_start_all_fiber_succ = false;
        TRPC_LOG_ERROR("StartFiberDetached to execute task:" << i << " faild, please retry latter");
      }
    }

    task_lists_[0].Process();
    fiber_latch.Wait();

    if (TRPC_UNLIKELY(!is_start_all_fiber_succ)) {
      return Status(-1, "some fiber entity create failed, return fail");
    }
    return kDefaultStatus;
  }

  return task_lists_[0].Process();
}

template <>
Status TrpcTaskExcutorImp<TaskExecutor<TrpcCoroutineTask>>::FiberExecTaskInNotFiberWorker() {
  size_t task_num = this->task_lists_.size();

  trpc::Latch l(task_num);
  bool is_start_all_fiber_succ = true;
  for (size_t i = 0; i < task_num; ++i) {
    bool ret = trpc::StartFiberDetached([&l, this, i] {
      this->task_lists_[i].Process();
      l.count_down();
    });

    if (TRPC_UNLIKELY(!ret)) {
      l.count_down();
      is_start_all_fiber_succ = false;
      TRPC_LOG_ERROR("StartFiberDetached to execute task:" << i << " faild, please retry latter");
    }
  }

  l.wait();

  if (TRPC_UNLIKELY(!is_start_all_fiber_succ)) {
      return Status(-1, "some fiber entity create failed, return fail");
  }

  return kDefaultStatus;
}

template <>
Status TrpcTaskExcutorImp<TaskExecutor<TrpcCoroutineTask>>::FiberExecTask() {
  if (IsRunningInFiberWorker()) {
    return FiberExecTaskInFiberWorker();
  }

  return FiberExecTaskInNotFiberWorker();
}

template <>
Status TrpcTaskExcutorImp<TaskExecutor<TrpcCoroutineTask>>::ExecTasksImp() {
  if (TRPC_UNLIKELY(this->task_lists_.empty())) {
    return kDefaultStatus;
  }

  if (runtime::GetRuntimeType() == runtime::kFiberRuntime) {
    return FiberExecTask();
  }

  TRPC_ASSERT(false && "cannot execute because not in fiber runtime.");

  return kDefaultStatus;
}

}  // namespace trpc::coroutine
