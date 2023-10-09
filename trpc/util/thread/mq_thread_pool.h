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

#pragma once

#include "trpc/util/function.h"
#include "trpc/util/thread/bounded_mpmc_queue.h"
#include "trpc/util/thread/mq_thread_pool_worker.h"
#include "trpc/util/thread/thread_pool_option.h"

namespace trpc {

/// @brief The implementation of task-steal thread pool
/// The difference between this class and `SQThreadPool` is:
/// this class there are two queues:
/// 1. shared task queue
/// 2. thread local queue
/// Tasks submitted by threads outside the thread pool are submitted to the shared queue,
/// and all threads in the thread pool can take tasks from it for execution.
/// The tasks produced in the threads in the thread pool are submitted to their own local private queues,
/// when other threads are idle, they can get tasks from the shared queue,
/// or steal tasks from the queues of other threads.
///
/// usage:
///
/// trpc::ThreadPoolOption thread_pool_option;
/// thread_pool_option.thread_num = 2;
/// trpc::MQThreadPool thread_pool(std::move(thread_pool_option));
/// thread_pool.Start();
/// thread_pool.PushTask([]());
/// thread_pool.Stop();
///
class MQThreadPool {
 public:
  static constexpr size_t kTaskQueueSize = 10000;

  explicit MQThreadPool(ThreadPoolOption&& thread_pool_option);

  /// @brief Set thread initialization function
  /// When the user needs to initialize a custom thread private variable
  /// for each worker thread, use this function
  void SetThreadInitFunc(Function<void()>&& func);

  /// @brief Start the thread pool
  bool Start();

  /// @brief Put a task into the thread pool
  /// @return true: success, false: failed(the reason is that the queue is full)
  bool AddTask(Function<void()>&& job);

  /// @brief Stop the thread pool
  void Stop();

  /// @brief wait for all threads to finish
  /// Empty implementation, the operation of waiting for thread finish has been implemented in `Stop`
  void Join() {}

  /// @brief Get thread pool option
  const ThreadPoolOption& GetThreadPoolOption() const { return thread_pool_option_; }

  /// @brief Check if the thread pool is in terminated state
  bool IsStop();

 private:
  void Spawn(size_t thread_num);
  void ExecuteTask(MQThreadPoolWorker& worker, MQThreadPoolTask*& task);
  void Invoke(MQThreadPoolWorker& worker, MQThreadPoolTask* task);
  bool WaitForTask(MQThreadPoolWorker& worker, MQThreadPoolTask*& task);
  void StealTask(MQThreadPoolWorker& worker, MQThreadPoolTask*& task);

 private:
  ThreadPoolOption thread_pool_option_;
  std::vector<MQThreadPoolWorker> workers_;
  std::vector<std::thread> threads_;

  PredicateNotifier notifier_;

  BoundedMPMCQueue<MQThreadPoolTask*> global_task_queue_;

  std::atomic<size_t> num_actives_{0};
  std::atomic<size_t> num_thieves_{0};
  std::atomic<bool> done_{0};

  Function<void()> thread_init_func_;
};

}  // namespace trpc
