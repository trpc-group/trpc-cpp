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

#include "trpc/transport/client/fiber/common/fiber_backup_request_retry.h"

#include <utility>

namespace trpc {

bool FiberBackupRequestRetry::IsReplyReady() {
  return get_reply_.exchange(true, std::memory_order_relaxed);
}

bool FiberBackupRequestRetry::IsFailedCountUpToAll() {
  return (failed_count_.fetch_add(1, std::memory_order_relaxed) == (retry_times_ - 1));
}

bool FiberBackupRequestRetry::IsFinished() { return is_finished_; }

void FiberBackupRequestRetry::SetFinished() {
  std::unique_lock<FiberMutex> _(mtx_);
  is_finished_ = true;
  cond_.notify_one();
}

bool FiberBackupRequestRetry::Wait(uint32_t timeout) {
  std::unique_lock<FiberMutex> _(mtx_);
  if (timeout > 0) {
    cond_.wait_for(_, std::chrono::milliseconds(timeout), [this]() { return is_finished_; });
  } else {
    cond_.wait(_, [this]() { return is_finished_; });
  }
  return is_finished_;
}

void FiberBackupRequestRetry::Reset() {
  get_reply_ = false;
  failed_count_ = 0;
  retry_times_ = 0;
  is_finished_ = false;
  if (deleter_) {
    deleter_();
    deleter_ = nullptr;
  }
}

void FiberBackupRequestRetry::SetResourceDeleter(ResourceDeleter&& deleter) {
  deleter_ = std::move(deleter);
}

}  // namespace trpc
