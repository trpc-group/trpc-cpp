//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/flow_control/recent_queue.h"

namespace trpc::overload_control {

RecentQueue::RecentQueue(int64_t limit, uint64_t window_size) : limit_(limit), window_size_(window_size) {
  cache_.resize(limit);
}

bool RecentQueue::Add() {
  std::unique_lock<std::mutex> lock(mutex_);
  auto now = trpc::time::GetSteadyNanoSeconds();
  if (now - window_size_ < cache_[cur_]) {
    return false;
  }
  cache_[cur_] = now;
  cur_ = (cur_ + 1) % limit_;
  return true;
}

int64_t RecentQueue::ActiveCount() {
  std::unique_lock<std::mutex> lock(mutex_);
  auto past = trpc::time::GetSteadyNanoSeconds() - window_size_;
  auto split = cache_.begin() + cur_;
  if (past < *split) {
    return limit_;
  }
  if (cache_.back() <= past) {
    return split - std::upper_bound(cache_.begin(), split, past);
  }
  return split - std::upper_bound(split, cache_.end(), past) + limit_;
}

}  // namespace trpc::overload_control

#endif
