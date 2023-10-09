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

#include "trpc/runtime/threadmodel/separate/io_worker_thread.h"

#include "trpc/util/thread/latch.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc {

IoWorkerThread::IoWorkerThread(Options&& options)
    : options_(std::move(options)),
      thread_(nullptr),
      reactor_(nullptr) {
}

void IoWorkerThread::Start() noexcept {
  Latch l(1);
  thread_ = std::make_unique<std::thread>([this, &l]() {
    ReactorImpl::Options reactor_options;
    reactor_options.id = this->GetUniqueId();
    reactor_options.max_task_queue_size = this->options_.io_task_queue_size;
    reactor_options.enable_async_io = this->options_.enable_async_io;
    reactor_options.io_uring_entries = this->options_.io_uring_entries;
    reactor_options.io_uring_flags = this->options_.io_uring_flags;

    this->reactor_ = std::make_unique<ReactorImpl>(reactor_options);
    TRPC_ASSERT(this->reactor_->Initialize());

    l.count_down();

    this->Run();
  });

  l.wait();
}

void IoWorkerThread::Stop() noexcept {
  reactor_->Stop();
}

void IoWorkerThread::Join() noexcept {
  if (thread_) {
    thread_->join();
  }
}

void IoWorkerThread::Destroy() noexcept {
  if (reactor_) {
    reactor_->Destroy();
    reactor_.reset();
  }
}

void IoWorkerThread::Run() noexcept {
  WorkerThread::SetCurrentWorkerThread(this);
  WorkerThread::SetCurrentThreadName(this);

  if (!options_.cpu_affinitys.empty()) {
    trpc::SetCurrentThreadAffinity(options_.cpu_affinitys);
  }

  reactor_->Run();
}

}  // namespace trpc
