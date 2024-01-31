
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

#include <atomic>
#include <cstdint>
#include <vector>

namespace trpc::naming {

/// @brief A thread-safe class for tracking invocation statistics using a sliding window implementation.
class BucketCircularArray {
 public:
  /// @brief Construct a bucket circular array
  /// @note It is necessary to ensure that stat_window_ms is divisible by buckets_num.
  BucketCircularArray(uint32_t stat_window_ms, uint32_t buckets_num);

  /// @brief Add statistical data
  void AddMetrics(uint64_t current_ms, bool success);

  /// @brief Clear statistical data
  void ClearMetrics();

  /// @brief Retrieve the failure rate within the statistical time period
  float GetErrorRate(uint64_t current_ms, uint32_t request_volume_threshold);

 private:
  struct Metrics {
    std::atomic<uint64_t> bucket_time{0};  // The start time of the bucket
    std::atomic<uint32_t> total_count{0};  // The request count during current time period
    std::atomic<uint32_t> error_count{0};  // The error count during current time period
  };

  uint32_t buckets_num_;
  uint32_t stat_window_ms_per_bucket_;
  std::vector<Metrics> buckets_;
};

}  // namespace trpc::naming