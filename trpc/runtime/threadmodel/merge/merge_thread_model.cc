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

#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool.h"
#include "trpc/util/random.h"

namespace trpc {

MergeThreadModel::MergeThreadModel(Options&& options) : options_(std::move(options)) {
  std::size_t io_cup_size = options_.cpu_affinitys.size();
  if (io_cup_size && options_.disallow_cpu_migration) {
    TRPC_ASSERT(io_cup_size >= options_.worker_thread_num);
  }

  for (std::size_t i = 0; i != options_.worker_thread_num; ++i) {
    MergeWorkerThread::Options worker_options;
    worker_options.group_name = options_.group_name;
    worker_options.group_id = options_.group_id;
    worker_options.id = i;
    worker_options.thread_model_type = kMerge;
    if (io_cup_size) {
      if (options_.disallow_cpu_migration) {
        worker_options.cpu_affinitys.push_back(options_.cpu_affinitys[i]);
      } else {
        worker_options.cpu_affinitys = options_.cpu_affinitys;
      }
    }

    worker_options.max_task_queue_size = options_.max_task_queue_size;
    worker_options.enable_async_io = options_.enable_async_io;
    worker_options.io_uring_entries = options_.io_uring_entries;
    worker_options.io_uring_flags = options_.io_uring_flags;

    worker_threads_.push_back(std::make_unique<MergeWorkerThread>(std::move(worker_options)));
  }
}

void MergeThreadModel::Start() noexcept {
  for (auto&& e : worker_threads_) {
    e->Start();
  }
}

void MergeThreadModel::Terminate() noexcept {
  for (auto&& e : worker_threads_) {
    e->Stop();
  }

  for (auto&& e : worker_threads_) {
    e->Join();
  }

  for (auto&& e : worker_threads_) {
    e->Destroy();
  }
}

bool MergeThreadModel::SubmitIoTask(MsgTask* io_task) noexcept {
  return SubmitMsgTask(io_task);
}

bool MergeThreadModel::SubmitHandleTask(MsgTask* handle_task) noexcept {
  return SubmitMsgTask(handle_task);
}

bool MergeThreadModel::SubmitMsgTask(MsgTask* task) noexcept {
  // To adapt the execution logic of multiple graphs in taskflow: all tasks must be fully asynchronous and delivered to
  // the queue for execution to avoid recursive locking leading to deadlocks. If it is a kParallelTask task, it will be
  // put into the task queue first before execution.
  if (task->task_type != runtime::kParallelTask) {
    if (TRPC_LIKELY(task->dst_thread_key < 0)) {
      WorkerThread* current = WorkerThread::GetCurrentWorkerThread();
      if (TRPC_LIKELY(current != nullptr)) {
        if (TRPC_LIKELY(current->GroupId() == task->group_id)) {
          task->handler();
          trpc::object_pool::Delete<MsgTask>(task);

          return true;
        }
      }
    }
  }

  uint16_t id = 0;
  if (task->dst_thread_key < 0) {
    thread_local static uint16_t index = 0;

    id = index++ % worker_threads_.size();
  } else {
    id = task->dst_thread_key % worker_threads_.size();
  }

  Reactor* reactor = worker_threads_[id]->GetReactor();

  bool ret = reactor->SubmitTask([task]() {
    task->handler();
    object_pool::Delete<MsgTask>(task);
  });

  if (!ret) {
    object_pool::Delete<MsgTask>(task);
  }

  return ret;
}

std::vector<Reactor*> MergeThreadModel::GetReactors() const noexcept {
  std::vector<Reactor*> reactors;
  for (auto&& e : worker_threads_) {
    reactors.push_back(e->GetReactor());
  }

  return reactors;
}

Reactor* MergeThreadModel::GetReactor(int index) const noexcept {
  if (index == -1) {
    index = trpc::Random();
  }

  index %= worker_threads_.size();

  return worker_threads_[index]->GetReactor();
}

}  // namespace trpc
