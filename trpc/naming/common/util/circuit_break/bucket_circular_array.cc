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

#include "trpc/naming/common/util/circuit_break/bucket_circular_array.h"

#include "trpc/util/log/logging.h"

namespace trpc::naming {

BucketCircularArray::BucketCircularArray(uint32_t stat_window_ms, uint32_t buckets_num)
    : buckets_num_(buckets_num), stat_window_ms_per_bucket_(stat_window_ms / buckets_num), buckets_(buckets_num) {
  for (auto& bucket : buckets_) {
    bucket.bucket_time.store(0, std::memory_order_relaxed);
    bucket.total_count.store(0, std::memory_order_relaxed);
    bucket.error_count.store(0, std::memory_order_relaxed);
  }
}

void BucketCircularArray::AddMetrics(uint64_t current_ms, bool success) {
  uint64_t bucket_time = current_ms / stat_window_ms_per_bucket_;
  int bucket_index = bucket_time % buckets_num_;
  auto& bucket = buckets_[bucket_index];
  // If it is data from the previous round, reset the data for that window.
  uint64_t store_bucket_time = bucket.bucket_time;
  if (bucket_time != store_bucket_time) {
    if (bucket.bucket_time.compare_exchange_weak(store_bucket_time, bucket_time, std::memory_order_relaxed)) {
      bucket.total_count.store(0, std::memory_order_relaxed);
      bucket.error_count.store(0, std::memory_order_relaxed);
    }
  }

  bucket.total_count.fetch_add(1, std::memory_order_relaxed);
  if (!success) {
    bucket.error_count.fetch_add(1, std::memory_order_relaxed);
  }
}

void BucketCircularArray::ClearMetrics() {
  // Since the time of data is checked when adding metrics, here we only need to reset the bucket_time.
  for (auto& bucket : buckets_) {
    bucket.bucket_time = 0;
  }
}

float BucketCircularArray::GetErrorRate(uint64_t current_ms, uint32_t request_volume_threshold) {
  uint64_t bucket_time = current_ms / stat_window_ms_per_bucket_;
  uint64_t error_count = 0;
  uint64_t total_count = 0;
  for (auto& bucket : buckets_) {
    // Only collect data from the most recent round.
    if (bucket.bucket_time.load(std::memory_order_relaxed) > (bucket_time - buckets_num_)) {
      total_count += bucket.total_count.load(std::memory_order_relaxed);
      error_count += bucket.error_count.load(std::memory_order_relaxed);
    }
  }

  if (total_count >= request_volume_threshold) {
    return static_cast<float>(error_count) / total_count;
  }

  // If the minimum number of requests is not reached, return a failure rate of 0.
  return 0;
}

}  // namespace trpc::naming
