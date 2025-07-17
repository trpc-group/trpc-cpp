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

#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"

#include <mutex>

#include "trpc/util/check.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool.h"
#include "trpc/util/random.h"

namespace trpc {

SeparateThreadModel::SeparateThreadModel(Options&& options) : options_(std::move(options)) {
  handle_scheduling_ = CreateSeparateScheduling(options_.scheduling_name);

  if (options_.disallow_cpu_migration &&
      (options_.io_thread_num > options_.io_cpu_affinitys.size() ||
        options_.handle_thread_num > options_.handle_cpu_affinitys.size())) {
    TRPC_CHECK(false,
               "CPU migration of fiber workers is explicitly disallowed, but there "
               "isn't enough CPU to dedicate one for each handle or io worker.");
  }

  for (std::size_t i = 0; i != options_.handle_thread_num; ++i) {
    HandleWorkerThread::Options worker_options;
    worker_options.group_name = options_.group_name;
    worker_options.group_id = options_.group_id;
    worker_options.id = i;
    worker_options.thread_model_type = kSeparate;
    worker_options.scheduling = handle_scheduling_.get();
    if (options_.disallow_cpu_migration) {
      worker_options.cpu_affinitys.push_back(options_.handle_cpu_affinitys[i]);
    } else {
      worker_options.cpu_affinitys = options_.handle_cpu_affinitys;
    }

    handle_threads_.push_back(std::make_unique<HandleWorkerThread>(std::move(worker_options)));
  }

  for (std::size_t i = 0; i != options_.io_thread_num; ++i) {
    IoWorkerThread::Options worker_options;
    worker_options.group_name = options_.group_name;
    worker_options.group_id = options_.group_id;
    worker_options.id = i;
    worker_options.io_task_queue_size = options_.io_task_queue_size;
    worker_options.enable_async_io = options_.enable_async_io;
    worker_options.io_uring_entries = options_.io_uring_entries;
    worker_options.io_uring_flags = options_.io_uring_flags;
    worker_options.thread_model_type = kSeparate;

    if (options_.disallow_cpu_migration) {
      worker_options.cpu_affinitys.push_back(options_.io_cpu_affinitys[i]);
    } else {
      worker_options.cpu_affinitys = options_.io_cpu_affinitys;
    }

    io_threads_.push_back(std::make_unique<IoWorkerThread>(std::move(worker_options)));
  }
}

void SeparateThreadModel::Start() noexcept {
  for (auto&& e : handle_threads_) {
    e->Start();
  }

  for (auto&& e : io_threads_) {
    e->Start();
  }
}

void SeparateThreadModel::Terminate() noexcept {
  handle_scheduling_->Stop();

  for (auto&& e : io_threads_) {
    e->Stop();
  }

  for (auto&& e : handle_threads_) {
    e->Stop();
  }

  for (auto&& e : io_threads_) {
    e->Join();
  }

  for (auto&& e : handle_threads_) {
    e->Join();
  }

  for (auto&& e : io_threads_) {
    e->Destroy();
  }

  for (auto&& e : handle_threads_) {
    e->Destroy();
  }

  handle_scheduling_->Destroy();
}

bool SeparateThreadModel::SubmitIoTask(MsgTask* io_task) noexcept {
  TRPC_ASSERT(io_task);
  TRPC_ASSERT(io_task->group_id == options_.group_id);

  uint16_t id = 0;

  if (io_task->dst_thread_key >= 0) {
    id = io_task->dst_thread_key % io_threads_.size();
  } else {
    thread_local static uint16_t index = 0;

    id = index++ % io_threads_.size();
  }

  Reactor* reactor = io_threads_[id]->GetReactor();
  TRPC_ASSERT(reactor);

  bool ret = reactor->SubmitTask([io_task]() {
    io_task->handler();
    object_pool::Delete<MsgTask>(io_task);
  });

  if (!ret) {
    object_pool::Delete<MsgTask>(io_task);
  }

  return ret;
}

bool SeparateThreadModel::SubmitHandleTask(MsgTask* handle_task) noexcept {
  TRPC_ASSERT(handle_task);

  return handle_scheduling_->SubmitHandleTask(handle_task);
}

std::vector<Reactor*> SeparateThreadModel::GetReactors() const noexcept {
  std::vector<Reactor*> reactors;
  for (auto&& e : io_threads_) {
    reactors.push_back(e->GetReactor());
  }

  return reactors;
}

Reactor* SeparateThreadModel::GetReactor(int index) const noexcept {
  if (index == -1) {
    index = trpc::Random();
  }

  index %= io_threads_.size();

  return io_threads_[index]->GetReactor();
}

}  // namespace trpc
