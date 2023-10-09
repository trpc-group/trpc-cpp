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

#include <cstdint>
#include <queue>
#include <functional>

#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

constexpr uint64_t kInvalidTimerId = std::numeric_limits<uint64_t>::max();

class TimerQueue;

using TimerExecutor = Function<void()>;

/// @brief The data structure of the timer task in normal thread mode
struct alignas(64) TimerTask : object_pool::EnableLwSharedFromThis<TimerTask> {
  /// @brief Timer id
  uint64_t timer_id = kInvalidTimerId;

  /// @brief Timer‘s owner
  TimerQueue* owner{nullptr};

  /// @brief Expire time
  uint64_t expiration = 0;

  /// @brief Repeat execution interval, 0 means no repeat execution
  uint64_t interval = 0;

  /// @brief Execute function
  TimerExecutor executor;

  /// @brief whether to create timer
  bool create = false;

  /// @brief whether to cancel timer
  bool cancel = false;

  /// @brief whether to pause timer
  bool pause = false;

  /// @brief whether to pause timer
  bool resume = false;
};

using TimerTaskPtr = object_pool::LwSharedPtr<TimerTask>;

namespace object_pool {

template <>
struct ObjectPoolTraits<TimerTask> {
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
