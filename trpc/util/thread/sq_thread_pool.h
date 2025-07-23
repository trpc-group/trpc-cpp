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
#include <thread>
#include <vector>

#include "trpc/util/function.h"
#include "trpc/util/thread/bounded_mpmc_queue.h"
#include "trpc/util/thread/futex_notifier.h"
#include "trpc/util/thread/thread_pool_option.h"

namespace trpc {

/// @brief The implementation of shared task queue thread pool
/// usage:
///
/// trpc::ThreadPoolOption option;
/// option.thread_num = 2;
/// trpc::SQThreadPool tp(std::move(option));
///
/// tp.AddTask([] {});
///
/// tp.Stop();
/// tp.Join();
///
class SQThreadPool {
 public:
  explicit SQThreadPool(ThreadPoolOption&& thread_pool_option);

  /// @brief Start the thread pool
  bool Start();

  /// @brief Put a task into the thread pool
  bool AddTask(Function<void()>&& job);

  /// @brief Stop the thread pool
  void Stop();

  /// @brief wait for all threads to finish
  void Join();

  /// @brief Get thread pool option
  const ThreadPoolOption& GetThreadPoolOption() const { return thread_pool_option_; }

  /// @brief Check if the thread pool is in terminated state
  bool IsStop();

 private:
  void Run();

 private:
  ThreadPoolOption thread_pool_option_;

  std::vector<std::thread> threads_;

  FutexNotifier futex_notifier_;
  BoundedMPMCQueue<Function<void()>> tasks_;
};

}  // namespace trpc
