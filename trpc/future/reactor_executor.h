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

#include <map>
#include <mutex>
#include <utility>

#include "trpc/future/executor.h"
#include "trpc/runtime/runtime.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief Executor to schedule future task into threads having reactor.
class ReactorExecutor : public Executor {
 public:
  /// @brief Get the current thread local ReactorExecutor instance,
  ///        which initailize by current thread local reactor.
  /// @return Executor.
  static inline Executor* ThreadLocal() {
    static thread_local ReactorExecutor executor(runtime::GetThreadLocalReactor());
    return &executor;
  }

  /// @brief Get the ReactorExecutor instance owned by thread which reactor indexed by index.
  /// @param index Id of ReactorExecutor instance.
  /// @return Executor.
  static Executor* Index(uint32_t index) {
    static std::mutex mutex;
    static std::map<uint32_t, ReactorExecutor> executors;

    std::lock_guard<std::mutex> lock(mutex);
    auto iter = executors.find(index);
    if (iter == executors.end()) {
      auto ret = executors.emplace(index, ReactorExecutor(runtime::GetReactor(index)));
      TRPC_ASSERT(ret.second);
      iter = ret.first;
    }

    return &(iter->second);
  }

 private:
  /// @brief Constructor by Reactor.
  explicit ReactorExecutor(Reactor* reactor) : reactor_(reactor) { TRPC_ASSERT(reactor_); }

  /// @brief Submit task to current reactor.
  bool SubmitTask(Task&& task) override { return reactor_->SubmitTask(std::move(task)); }

 private:
  Reactor* reactor_ = nullptr;
};

}  // namespace trpc
