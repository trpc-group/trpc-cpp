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

#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "trpc/common/status.h"

namespace trpc::coroutine {  // It's not necessary to make namespace named "coroutine" public.

/// @brief Base executor for multiple fiber tasks, run in fiber runtime,
/// it can use in fiber worker thread and non-fiber worker thread.
///
/// for examples:
/// @code{.cpp}
/// TrpcTaskExecutor task_executor;
/// task_executor.AddTask([]{});
/// task_executor.AddTask([]{});
/// task_executor.AddTask([]{});
/// ...
/// Status status = task_executor.ExecTasks();
/// if (!status.OK()) {
///    const auto& results = task_executor.GetTaskList();
///    ....
/// }
/// @endcode
///
template <typename Task>
class TaskExecutor {
 public:
  enum Stat {
    kAddTask,  // Add task state
    kRunTask,  // Run task state
  };

  virtual ~TaskExecutor() { Clear(); }

  void AddTask(std::function<void()>&& func) {
    if (stat_ != kAddTask) {
      Clear();
      stat_ = kAddTask;
    }

    task_lists_.emplace_back(Task(std::move(func)));
  }

  template <typename Functor, typename... Args>
  void AddTask(Functor&& func, Args&&... args) {
    if (stat_ != kAddTask) {
      Clear();
      stat_ = kAddTask;
    }

    Task task([func = std::forward<Functor>(func),
               args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
      return std::apply(func, std::move(args));
    });

    task_lists_.emplace_back(std::move(task));
  }

  Status ExecTasks() {
    stat_ = kRunTask;
    return ExecTasksImp();
  }

  void Clear() { task_lists_.clear(); }

  const std::vector<Task>& GetTaskList() const { return task_lists_; }

 protected:
  virtual Status ExecTasksImp() = 0;

 protected:
  Stat stat_{kAddTask};
  std::vector<Task> task_lists_;
};

}  // namespace trpc::coroutine
