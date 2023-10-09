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

#include "trpc/util/thread/sq_thread_pool.h"

#include <utility>

#include "trpc/util/bind_core_manager.h"
#include "trpc/util/log/logging.h"

namespace trpc {

SQThreadPool::SQThreadPool(ThreadPoolOption&& thread_pool_option)
    : thread_pool_option_(std::move(thread_pool_option)) {}

bool SQThreadPool::Start() {
  TRPC_ASSERT(thread_pool_option_.thread_num > 0);
  TRPC_ASSERT(thread_pool_option_.task_queue_size > 0);

  tasks_.Init(thread_pool_option_.task_queue_size);

  threads_.resize(thread_pool_option_.thread_num);

  for (auto&& t : threads_) {
    t = std::thread([this] {
      if (this->thread_pool_option_.bind_core) {
        BindCoreManager::BindCore();
      }
      this->Run();
    });
  }

  return true;
}

bool SQThreadPool::AddTask(Function<void()>&& job) {
  if (IsStop()) {
    return false;
  }

  if (tasks_.Push(std::move(job))) {
    futex_notifier_.Wake(1);
    return true;
  }

  return false;
}

void SQThreadPool::Stop() { futex_notifier_.Stop(); }

void SQThreadPool::Join() {
  for (auto&& t : threads_) {
    t.join();
  }
}

bool SQThreadPool::IsStop() {
  const trpc::FutexNotifier::State st = futex_notifier_.GetState();
  return st.Stopped();
}

void SQThreadPool::Run() {
  Function<void()> task;
  while (true) {
    if (tasks_.Pop(task)) {
      task();
    } else {
      const trpc::FutexNotifier::State st = futex_notifier_.GetState();
      if (st.Stopped()) {
        break;
      }
      futex_notifier_.Wait(st);
    }
  }

  while (true) {
    if (tasks_.Pop(task)) {
      task();
    } else {
      break;
    }
  }
}

}  // namespace trpc
