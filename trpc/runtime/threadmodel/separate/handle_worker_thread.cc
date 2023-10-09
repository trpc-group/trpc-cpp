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

#include "trpc/runtime/threadmodel/separate/handle_worker_thread.h"

#include <string>
#include <utility>

#include "trpc/util/thread/latch.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc {

HandleWorkerThread::HandleWorkerThread(Options&& options)
    : options_(std::move(options)),
      thread_(nullptr) {
  TRPC_ASSERT(options_.scheduling != nullptr);
}

void HandleWorkerThread::Start() noexcept {
  Latch l(1);
  thread_ = std::make_unique<std::thread>([this, &l]() {
    l.count_down();
    this->Run();
  });

  l.wait();
}

void HandleWorkerThread::Stop() noexcept {
  options_.scheduling->Stop();
}

void HandleWorkerThread::Join() noexcept {
  if (thread_) {
    thread_->join();
  }
}

void HandleWorkerThread::Destroy() noexcept {}

void HandleWorkerThread::ExecuteTask() noexcept {
  options_.scheduling->ExecuteTask(options_.scheduling->GetCurrentWorkerIndex());
}

void HandleWorkerThread::Run() noexcept {
  WorkerThread::SetCurrentWorkerThread(this);
  WorkerThread::SetCurrentThreadName(this);

  if (!options_.cpu_affinitys.empty()) {
    trpc::SetCurrentThreadAffinity(options_.cpu_affinitys);
  }

  options_.scheduling->Enter(options_.id);
  options_.scheduling->Schedule();
  options_.scheduling->Leave();
}

}  // namespace trpc
