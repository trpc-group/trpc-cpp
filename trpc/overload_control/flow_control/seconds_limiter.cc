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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/flow_control/seconds_limiter.h"

#include <thread>

#include "trpc/overload_control/common/report.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

SecondsLimiter::SecondsLimiter(int64_t limit, bool is_report, int32_t window_size)
    : limit_(limit), is_report_(is_report), window_size_(window_size <= 0 ? kDefaultWindowSize : window_size) {
  TRPC_ASSERT(window_size_ > 0);
  // init counter
  counters_ = std::make_unique<SecondsCounter[]>(window_size_);
  for (int32_t i = 0; i < window_size_; ++i) {
    counters_[i].counter = 0;
    counters_[i].access_timestamp = 0;
  }
}

bool SecondsLimiter::CheckLimit(const ServerContextPtr& context) {
  // lock version
  auto nows = trpc::time::GetMilliSeconds() / 1000;
  size_t bucket = nows % window_size_;
  int64_t bucket_access_time = counters_[bucket].access_timestamp.load(std::memory_order_relaxed);

  if (bucket_access_time != static_cast<int64_t>(nows)) {
    // reset counter
    std::unique_lock<std::mutex> locker(mutex_);
    bucket_access_time = counters_[bucket].access_timestamp.load(std::memory_order_acquire);
    if (bucket_access_time != static_cast<int64_t>(nows)) {
      counters_[bucket].counter.fetch_and(0, std::memory_order_relaxed);
      counters_[bucket].access_timestamp.store(nows, std::memory_order_release);
    }
  }
  int64_t result = counters_[bucket].counter.fetch_add(1, std::memory_order_relaxed);
  bool ret = result + 1 > limit_;
  if (is_report_) {
    OverloadInfo infos;
    infos.attr_name = "SecondsLimiter";
    infos.report_name = context->GetFuncName();
    infos.tags["current_qps"] = result + 1;
    infos.tags["max_qps"] = limit_;
    infos.tags["window_size"] = window_size_;
    infos.tags[kOverloadctrlPass] = (ret ? 0 : 1);
    infos.tags[kOverloadctrlLimited] = (ret ? 1 : 0);
    Report::GetInstance()->ReportOverloadInfo(infos);
  }
  return ret;
}

int64_t SecondsLimiter::GetCurrCounter() {
  auto nows = trpc::time::GetMilliSeconds() / 1000;
  size_t bucket = nows % window_size_;
  int64_t last_access_timestmap = counters_[bucket].access_timestamp.load(std::memory_order_relaxed);
  if (last_access_timestmap != static_cast<int64_t>(nows)) {
    return 0;
  }
  return counters_[bucket].counter.load(std::memory_order_relaxed);
}

}  // namespace trpc::overload_control

#endif
