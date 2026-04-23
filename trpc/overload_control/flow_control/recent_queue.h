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

#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include "trpc/util/time.h"

namespace trpc::overload_control {

class RecentQueue {
 public:
  explicit RecentQueue(int64_t limit, uint64_t window_size);

  bool Add();

  int64_t ActiveCount();

 private:
  std::vector<uint64_t> cache_;
  std::mutex mutex_;

  // The location for cache eviction is also where the new timestamp is.
  int64_t cur_{0};

  // Maximum number of requests per second.
  int64_t limit_;
  // The time window size for cache eviction.
  uint64_t window_size_;
};

using RecentQueuePtr = std::unique_ptr<RecentQueue>;

}  // namespace trpc::overload_control

#endif
