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

#include "trpc/runtime/common/heartbeat/heartbeat_info.h"

#include <sys/syscall.h>
#include <unistd.h>

#include <shared_mutex>
#include <utility>

#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

std::shared_mutex thread_heartbeat_function_mutex_;
ThreadHeartBeatFunction thread_heartbeat_function{nullptr};

void RegisterThreadHeartBeatFunction(ThreadHeartBeatFunction&& heartbeat_function) {
  std::unique_lock<std::shared_mutex> lock(thread_heartbeat_function_mutex_);
  thread_heartbeat_function = std::move(heartbeat_function);
}

void HeartBeat(uint32_t task_queue_size) {
  std::shared_lock<std::shared_mutex> lock(thread_heartbeat_function_mutex_);
  if (TRPC_UNLIKELY(!thread_heartbeat_function)) {
    return;
  }

  thread_local pid_t thread_pid = ::syscall(SYS_gettid);
  thread_local uint64_t last_heartbeat_time_ms = 0;

  auto now_ms = trpc::time::GetMilliSeconds();
  if (now_ms >= last_heartbeat_time_ms + 3000) {
    WorkerThread* worker_thread = WorkerThread::GetCurrentWorkerThread();
    TRPC_ASSERT(worker_thread != nullptr);

    ThreadHeartBeatInfo thread_heart_beat_info;
    thread_heart_beat_info.group_name = worker_thread->GetGroupName();
    thread_heart_beat_info.role = worker_thread->Role();
    thread_heart_beat_info.id = worker_thread->Id();
    thread_heart_beat_info.pid = thread_pid;
    thread_heart_beat_info.last_time_ms = now_ms;
    thread_heart_beat_info.task_queue_size = task_queue_size;

    thread_heartbeat_function(std::move(thread_heart_beat_info));

    last_heartbeat_time_ms = now_ms;
  }
}

}  // namespace trpc
