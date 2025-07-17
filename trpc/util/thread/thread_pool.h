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

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "trpc/util/function.h"
#include "trpc/util/thread/mq_thread_pool.h"
#include "trpc/util/thread/sq_thread_pool.h"
#include "trpc/util/thread/thread_pool_option.h"

namespace trpc {

/// @brief Implementation of thread pool
/// usage:
///
/// trpc::ThreadPoolOption thread_pool_option;
/// thread_pool_option.thread_num = 2;
/// trpc::ThreadPoolImpl<trpc::MQThreadPool> thread_pool(std::move(thread_pool_option));
///
/// thread_pool.Start();
//
/// thread_pool.AddTask([]() {});
///
/// thread_pool.Stop();
/// thread_pool.Join();
///
template <typename T>
class ThreadPoolImpl {
 public:
  explicit ThreadPoolImpl(ThreadPoolOption&& thread_pool_option) {
    thread_pool_ = std::make_unique<T>(std::move(thread_pool_option));
  }

  /// @brief Start the thread pool
  bool Start() { return thread_pool_->Start(); }

  /// @brief Put a task into the thread pool
  bool AddTask(Function<void()>&& job) { return thread_pool_->AddTask(std::move(job)); }

  /// @brief Stop the thread pool
  void Stop() { thread_pool_->Stop(); }

  /// @brief wait for all threads to finish
  void Join() { thread_pool_->Join(); }

  /// @brief Get thread pool option
  const ThreadPoolOption& GetThreadPoolOption() const { return thread_pool_->GetThreadPoolOption(); }

  /// @brief Check if the thread pool is in terminated state
  bool IsStop() { return thread_pool_->IsStop(); }

 private:
  std::unique_ptr<T> thread_pool_;
};

// for compatible
using ThreadPool = ThreadPoolImpl<SQThreadPool>;

}  // namespace trpc
