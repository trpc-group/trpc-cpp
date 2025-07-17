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

#include "trpc/runtime/threadmodel/common/task_type.h"
#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool.h"

namespace trpc {

// struct MsgTask;
using MsgTaskHandler = Function<void()>;

/// @brief Network message tasks for request/response in separate and merge thread mode
struct MsgTask {
  /// task type defined in task_type.h
  uint16_t task_type;

  /// used to dispatch tasks to a specified thread model for processing.
  uint16_t group_id;

  /// Routing key for dispatching to a specific task processing thread under a thread model instance.
  /// In no specifed case, if the thread model id that the currenct thread belongs to is equal to group_id,
  /// then the task will be processed in the current thread. Otherwise, a random thread under the specified
  /// thread model for processing is selected.
  int32_t dst_thread_key = -1;

  /// related parameters for task processing
  void* param = nullptr;

  /// task processing method
  MsgTaskHandler handler;
};

namespace object_pool {

template <>
struct ObjectPoolTraits<MsgTask> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

}  // namespace trpc
