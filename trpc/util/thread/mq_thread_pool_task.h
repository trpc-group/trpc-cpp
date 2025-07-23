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

#include "trpc/util/function.h"

namespace trpc {

using MQThreadPoolTaskFunc = Function<void()>;

class MQThreadPoolTask {
  friend class MQThreadPool;

 public:
  explicit MQThreadPoolTask(MQThreadPoolTask&& task) : task_func_(std::move(task.task_func_)) {}

  explicit MQThreadPoolTask(MQThreadPoolTaskFunc&& task_func) : task_func_(std::move(task_func)) {}

  MQThreadPoolTask& operator=(MQThreadPoolTask&& task) {
    task_func_ = std::move(task.task_func_);
    return *this;
  }

 private:
  MQThreadPoolTaskFunc task_func_;
};

}  // namespace trpc
