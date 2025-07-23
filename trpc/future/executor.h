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

/// @brief Base class for how to execute future task.
class Executor {
 public:
  using Task = Function<void()>;

  virtual ~Executor() = default;

  /// @brief Overrided by subclass to determined where future ready task is executed.
  virtual bool SubmitTask(Task&&) = 0;
};

/// @brief Executed future task inside current thread.
/// @note Maybe useful for perf or some other cases.
class InlineExecutor final : public Executor {
 public:
  inline bool SubmitTask(Task&& task) override {
    task();
    return true;
  }
};

static inline InlineExecutor kDefaultInlineExecutor;

}  // namespace trpc
