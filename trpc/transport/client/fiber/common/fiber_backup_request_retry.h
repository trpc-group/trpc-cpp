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

#include <atomic>
#include <functional>
#include <limits>
#include <mutex>

#include "trpc/transport/client/retry_info_def.h"
#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

/// @brief The implementation for fiber backup request retry
class FiberBackupRequestRetry : public BackupRequestRetryBase, public object_pool::LwSharedPtrCountBase {
  using ResourceDeleter = Function<void()>;

 public:
  ~FiberBackupRequestRetry() override {
    if (deleter_) {
      deleter_();
      deleter_ = nullptr;
    }
  }

  bool IsReplyReady() override;

  bool IsFailedCountUpToAll() override;

  bool IsFinished() override;

  void SetFinished() override;

  bool Wait(uint32_t timeout = 0) override;

  void SetResourceDeleter(ResourceDeleter&& deleter);

  void Reset();

  void SetRetryTimes(int times) { retry_times_ = times; }

 private:
  std::atomic_uint32_t failed_count_{0};
  uint32_t retry_times_{0};
  std::atomic_bool get_reply_{false};
  FiberMutex mtx_;
  FiberConditionVariable cond_;
  bool is_finished_{false};
  ResourceDeleter deleter_{nullptr};
};

namespace object_pool {

template <>
struct ObjectPoolTraits<FiberBackupRequestRetry> {
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
