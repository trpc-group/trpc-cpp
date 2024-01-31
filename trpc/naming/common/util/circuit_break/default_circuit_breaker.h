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
#include <deque>
#include <mutex>
#include <numeric>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "trpc/naming/common/util/circuit_break/bucket_circular_array.h"
#include "trpc/naming/common/util/circuit_break/circuit_breaker.h"
#include "trpc/naming/common/util/circuit_break/default_circuit_breaker_config.h"

namespace trpc::naming {

/// @brief The implementation of the default circuit breaker.
class DefaultCircuitBreaker : public CircuitBreaker {
 public:
  static constexpr char kName[] = "default";

  DefaultCircuitBreaker(const DefaultCircuitBreakerConfig& config, const std::string& service_name);

  void Reserve(const std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash>& keys) override;

  bool StatusChanged(uint64_t current_ms) override;

  void AddRecordData(const CircuitBreakRecordKey& key, uint64_t current_ms, bool success) override;

  std::unordered_map<CircuitBreakRecordKey, CircuitBreakStatus, CircuitBreakRecordKeyHash> GetAllStatus() override;

 private:
  struct CircuitBreakRecordData {
    CircuitBreakRecordData(uint32_t stat_window_ms, uint32_t buckets_num) : metrics_data(stat_window_ms, buckets_num) {}
    BucketCircularArray metrics_data;                                    // invoke statistics
    std::atomic<CircuitBreakStatus> status{CircuitBreakStatus::kClose};  // circuit break state
    std::atomic<uint32_t> continuous_error_count{0};         // The number of consecutive failed times in closed state
    std::atomic<uint64_t> circuitbreak_open_time{0};         // The moment when switch to the open state
    std::atomic<uint32_t> success_count_after_half_open{0};  // The success count in half-open state
    std::atomic<uint32_t> request_count_after_half_open{0};  // The request count in half-open state
    std::atomic<uint64_t> current_check_error_rate_time{0};  // The recent moment of checking if the failure rate has
                                                             // exceeded the threshold.
  };

  void SwitchStatus(const CircuitBreakRecordKey& key, CircuitBreakRecordData& record_info, uint64_t current_ms,
                    CircuitBreakStatus from_status, CircuitBreakStatus to_status, int reason);

  void SetStatusChanged(bool value) { status_changed_.store(value, std::memory_order_relaxed); }

  void AddLastCircuitBreakOpenTime(uint64_t current_ms) {
    std::lock_guard<std::mutex> lock(switch_to_open_times_mutex_);
    switch_to_open_times_.push_back(current_ms);
  }

  bool ShouldChangeToHalfOpen(uint64_t current_ms);

 private:
  DefaultCircuitBreakerConfig config_;
  std::string service_name_;
  uint32_t stat_window_ms_per_bucket_;
  std::atomic<bool> status_changed_{false};

  // The status of node circuit breaker, where the key is ip:port
  std::unordered_map<CircuitBreakRecordKey, std::unique_ptr<CircuitBreakRecordData>, CircuitBreakRecordKeyHash>
      record_info_;
  mutable std::shared_mutex mutex_;

  std::deque<uint64_t> switch_to_open_times_;
  std::mutex switch_to_open_times_mutex_;
};

}  // namespace trpc::naming
