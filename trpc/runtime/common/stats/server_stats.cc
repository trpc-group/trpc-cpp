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

#include "trpc/runtime/common/stats/server_stats.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void ServerStats::AddConnCount(int count) { conn_count_.Update(count); }

void ServerStats::AddReqCount(uint64_t count) {
  total_req_count_.Add(count);
  req_count_.Add(count);
}

void ServerStats::AddFailedReqCount(uint64_t count) {
  total_failed_req_count_.Add(count);
  failed_req_count_.Add(count);
}

void ServerStats::AddReqDelay(uint64_t delay_in_ms) {
  total_req_delay_.Add(delay_in_ms);
  req_delay_.Add(delay_in_ms);
  max_delay_.Update(delay_in_ms);
}

void ServerStats::Stats() {
  uint64_t req_count = req_count_.Reset();
  uint64_t last_req_count = last_req_count_.exchange(req_count, std::memory_order_relaxed);
  uint64_t failed_req_count = failed_req_count_.Reset();
  uint64_t last_failed_req_count = last_failed_req_count_.exchange(failed_req_count, std::memory_order_relaxed);
  uint64_t req_delay = req_delay_.Reset();
  uint64_t last_req_delay = last_req_delay_.exchange(req_delay, std::memory_order_relaxed);
  uint64_t max_delay = max_delay_.Reset();
  uint64_t last_max_delay = last_max_delay_.exchange(max_delay, std::memory_order_relaxed);

  double avg_req_delay = 0.0;
  if (req_count > 0) {
    avg_req_delay = req_delay * 1.0 / req_count;
  }
  double avg_last_req_delay = 0.0;
  if (last_req_count > 0) {
    avg_last_req_delay = last_req_delay * 1.0 / last_req_count;
  }

  TRPC_LOG_INFO("\nconn_count: " << GetConnCount() << " last_max_delay: " << last_max_delay
                                 << " max_delay: " << max_delay << " avg_total_req_delay: " << GetAvgTotalDelay()
                                 << " avg_last_req_delay: " << avg_last_req_delay << " avg_req_delay: " << avg_req_delay
                                 << "\ntotal_req_count: " << GetTotalReqCount() << " last_req_count: " << last_req_count
                                 << " req_count: " << req_count << " total_failed_req_count: "
                                 << GetTotalFailedReqCount() << " last_failed_req_count: " << last_failed_req_count
                                 << " failed_req_count: " << failed_req_count);
}

}  // namespace trpc
