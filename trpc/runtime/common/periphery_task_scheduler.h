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

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "trpc/util/function.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Class used to execute periodic/non-periodic tasks.
///        Mainly used to converge scattered threads in the framework. When used internally in the framework, use the
///        interface with 'Inner' in its name. Framework users can also submit their own tasks using this class.
///        Generally, tasks can be submitted using 'SubmitTask/SubmitPeriodicalTask', and there is no need to manually
///        stop the tasks. The framework will stop all tasks uniformly when the program ends.
///        If there is a need to control the task stop, 'RemoveTask or StopTask/JoinTask' can be used, you can see
///        interface description for details.
///        The size of the thread group used by the framework user is controlled by the
///        'global->periphery_task_scheduler_thread_num' field in the YAML file, which defaults to 1 and can be
///        customized. The size of the thread group used by the framework can also be controlled by the
///        'global->inner_periphery_task_scheduler_thread_num' field in the YAML file, which defaults to 2.
///        Generally, does not need to be concerned about this value. If too many tasks
///        are started due to the use of many plugins, the value can be appropriately increased based on the performance
///        of the program.

class PeripheryTaskScheduler {
 public:
  PeripheryTaskScheduler() = default;
  ~PeripheryTaskScheduler();

  /// @brief Get the global instance
  static PeripheryTaskScheduler* GetInstance();

  /// @private
  bool Init();

  /// @private
  void Start();

  /// @private
  void Stop();

  /// @private
  void Join();

  /// @brief Submit a nonperiodic task
  /// @param task processing function of task
  /// @param name task name, the default value is "", it can be used for debugging
  /// @return on success, return the task id. on error, return 0
  std::uint64_t SubmitTask(Function<void()>&& task, const std::string& name = "");

  /// @brief Submit a periodic task
  /// @param task processing function of task
  /// @param name task name, the default value is "", it can be used for debugging
  /// @param interval periodic interval in milliseconds
  /// @return on success, return the task id. on error, return 0
  std::uint64_t SubmitPeriodicalTask(Function<void()>&& task, std::uint64_t interval, const std::string& name = "");

  /// @brief Submit a periodic task. The execution interval of scheduled tasks can be customized.
  /// @param task processing function of task
  /// @param name task name, the default value is "", it can be used for debugging
  /// @param interval_gen_func execution interval generation function in milliseconds
  /// @return on success, return the task id. on error, return 0
  std::uint64_t SubmitPeriodicalTask(Function<void()>&& task, Function<std::uint64_t()>&& interval_gen_func,
                                     const std::string& name = "");

  /// @brief Stop the task, used in conjunction with JoinTask to wait for tasks to complete before exiting.
  /// @param task_id task id
  /// @return on success, return true. on error, return false
  /// @note This interface can only be called once with the same ID.
  bool StopTask(std::uint64_t task_id);

  /// @brief Block and wait for tasks to complete, used in scenarios where it is necessary to wait for tasks to complete
  ///        before exiting.
  /// @param task_id task id
  /// @return on success, return the task id. on error, return 0
  /// @note 1. This interface cannot be called within the task, therefore if it is called before the task completes,
  ///          it will cause a deadlock.
  ///       2. This interface can only be called once with the same ID.
  bool JoinTask(std::uint64_t task_id);

  /// @brief Remove task, used in scenarios where it is not necessary to wait for tasks to complete before exiting.
  /// @param task_id task id
  /// @return on success, return true. on error, return false
  /// @note This interface can only be called once with the same ID.
  bool RemoveTask(std::uint64_t task_id);

  /// @brief Same as 'SubmitTask', but is used only internally by the framework.
  std::uint64_t SubmitInnerTask(Function<void()>&& task, const std::string& name = "");

  /// @brief Same as 'SubmitPeriodicalTask', but is used only internally by the framework.
  std::uint64_t SubmitInnerPeriodicalTask(Function<void()>&& task, std::uint64_t interval,
                                          const std::string& name = "");

  /// @brief Same as 'SubmitPeriodicalTask', but is used only internally by the framework.
  std::uint64_t SubmitInnerPeriodicalTask(Function<void()>&& task, Function<std::uint64_t()> interval_gen_func,
                                          const std::string& name = "");

  /// @brief Same as 'RemoveTask', but is used only internally by the framework.
  bool RemoveInnerTask(std::uint64_t task_id);

  /// @brief Same as 'Stoptask', but is used only internally by the framework.
  bool StopInnerTask(std::uint64_t task_id);

  /// @brief Same as 'Jointask', but is used only internally by the framework.
  bool JoinInnerTask(std::uint64_t task_id);

  /// @brief Used to destroy resources accessed by scheduled tasks after all scheduled task execution threads have
  ///        exited.
  /// @note Used only internally by the framework.
  void RegisterInnerResourceCleanCallback(Function<void()>&& cb) {
    if (inner_scheduler_) {
      inner_scheduler_->RegisterResourceCleanCallback(std::move(cb));
    }
  }

  PeripheryTaskScheduler(const PeripheryTaskScheduler&) = delete;

  PeripheryTaskScheduler& operator=(const PeripheryTaskScheduler&) = delete;

 private:
  // Concrete implementation of PeripheryTaskScheduler.
  class PeripheryTaskSchedulerImpl {
   public:
    PeripheryTaskSchedulerImpl() = default;
    ~PeripheryTaskSchedulerImpl();

    PeripheryTaskSchedulerImpl(const PeripheryTaskSchedulerImpl&) = delete;
    PeripheryTaskSchedulerImpl& operator=(const PeripheryTaskSchedulerImpl&) = delete;

    // submit a nonperiodic task
    std::uint64_t SubmitTaskImpl(Function<void()>&& task, const std::string& name = "");
    // submit a periodic task
    std::uint64_t SubmitPeriodicalTaskImpl(Function<void()>&& task, std::uint64_t interval,
                                           const std::string& name = "");
    std::uint64_t SubmitPeriodicalTaskImpl(Function<void()>&& task, Function<std::uint64_t()>&& interval_gen_func,
                                           const std::string& name = "");
    bool StopTaskImpl(std::uint64_t task_id);
    bool JoinTaskImpl(std::uint64_t task_id);
    bool RemoveTaskImpl(std::uint64_t task_id);

    void Init(size_t thread_num);
    void Start();
    void Stop();
    void Join();

    void RegisterResourceCleanCallback(Function<void()>&& cb) { clean_cb_ = std::move(cb); }

   private:
    struct Task : trpc::RefCounted<Task> {
      // task name
      std::string name;
      // is periodic task or not
      bool periodic = false;
      // is task been cancelled
      bool cancelled = false;
      // is task in running
      bool running = false;
      // is task execute completed
      bool done = false;
      // task expiration time
      std::chrono::steady_clock::time_point expires_at;
      // execution interval generation function in milliseconds
      // The design here takes into account scenarios where the interval between some periodic tasks may change,
      // such as the sampler started by tvar.
      Function<std::uint64_t()> interval_gen_func;
      // execute function of task
      Function<void()> cb;
      // used to ensure the safety of the internal state under multi-threaded conditions
      std::mutex lock;
      // used for external calls to wait for task execution to complete
      std::condition_variable stop_cv;
    };
    using TaskPtr = trpc::RefPtr<Task>;
    struct TaskPtrComp {
      bool operator()(const TaskPtr& p1, const TaskPtr& p2) const;
    };

    void Schedule();

    void TaskProc(TaskPtr&& task_ptr);

    std::uint64_t CreateAndSubmitTask(Function<void()>&& cb, Function<std::uint64_t()>&& interval_gen_func,
                                      bool periodic, const std::string& name);

    bool StopAndDestroyTask(std::uint64_t task_id, bool is_destroy);

    TaskPtr GetTaskPtr(std::uint64_t task_id);

    std::mutex lock_;

    std::condition_variable cv_;

    // the threads been asked to stop
    std::atomic<bool> exited_{false};

    // task queue
    std::priority_queue<TaskPtr, std::vector<TaskPtr>, TaskPtrComp> tasks_;

    // worker threads
    std::vector<std::thread> workers_;

    // number of threads
    size_t thread_num_{1};

    Function<void()> clean_cb_{nullptr};
  };

  std::unique_ptr<PeripheryTaskSchedulerImpl> scheduler_{nullptr}, inner_scheduler_{nullptr};

  std::atomic<bool> started_{false};
};

}  // namespace trpc
