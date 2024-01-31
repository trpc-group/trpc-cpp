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

#include "trpc/naming/common/util/circuit_break/default_circuit_breaker.h"

#include "trpc/util/log/logging.h"

namespace trpc::naming {

namespace {
const char* StatusStr(CircuitBreakStatus status) {
  switch (status) {
    case CircuitBreakStatus::kClose:
      return "Close";
    case CircuitBreakStatus::kHalfOpen:
      return "HalfOpen";
    case CircuitBreakStatus::kOpen:
      return "Open";
    default:
      return "Unknown";
  }
}

enum kReasonIndex {
  kContinuousError = 0,
  kExcessiveErrorRate,
  kEnoughSuccessCount,
  kNoEnoughSuccessCount,
  kReachSleepWindow,
};

const char* kReasonStr[] = {
    "continuous error",
    "excessive error rate",
    "enough success count after half-open",
    "no enough success count after half-open",
    "reach the sleep window point",
};

void PrintSwitchLog(const std::string& service_name, const CircuitBreakRecordKey& key, CircuitBreakStatus from_status,
                    CircuitBreakStatus to_status, int reason) {
  TRPC_FMT_INFO("{} {}:{} switch from {} status to {} status due to {}", service_name, key.ip, key.port,
                StatusStr(from_status), StatusStr(to_status), kReasonStr[reason]);
}
}  // namespace

DefaultCircuitBreaker::DefaultCircuitBreaker(const DefaultCircuitBreakerConfig& config, const std::string& service_name)
    : config_(config), service_name_(service_name) {
  config_.continuous_error_threshold -= 1;
  stat_window_ms_per_bucket_ = config_.stat_window_ms / config_.buckets_num;
}

void DefaultCircuitBreaker::Reserve(const std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash>& keys) {
  if (!config_.enable) {
    return;
  }

  std::unique_lock<std::shared_mutex> wl(mutex_);
  for (auto& key : keys) {
    if (record_info_.find(key) == record_info_.end()) {
      record_info_.emplace(key, std::make_unique<CircuitBreakRecordData>(config_.stat_window_ms, config_.buckets_num));
    }
  }

  for (auto iter = record_info_.begin(); iter != record_info_.end();) {
    if (keys.find(iter->first) == keys.end()) {
      iter = record_info_.erase(iter);
    } else {
      iter++;
    }
  }
}

bool DefaultCircuitBreaker::StatusChanged(uint64_t current_ms) {
  if (!config_.enable) {
    return false;
  }

  auto status_change = true;
  if (status_changed_.compare_exchange_weak(status_change, false, std::memory_order_relaxed)) {
    return true;
  }

  return ShouldChangeToHalfOpen(current_ms);
}

void DefaultCircuitBreaker::SwitchStatus(const CircuitBreakRecordKey& key, CircuitBreakRecordData& record_info,
                                         uint64_t current_ms, CircuitBreakStatus from_status,
                                         CircuitBreakStatus to_status, int reason) {
  if (to_status == CircuitBreakStatus::kOpen) {
    if (record_info.status.compare_exchange_strong(from_status, to_status, std::memory_order_relaxed)) {
      PrintSwitchLog(service_name_, key, from_status, to_status, reason);
      record_info.status.store(CircuitBreakStatus::kOpen);
      record_info.circuitbreak_open_time.store(current_ms);
      record_info.continuous_error_count.store(0);
      record_info.success_count_after_half_open.store(0);
      record_info.request_count_after_half_open.store(0);
      SetStatusChanged(true);
      AddLastCircuitBreakOpenTime(current_ms);
      return;
    }
  } else if (to_status == CircuitBreakStatus::kClose) {
    PrintSwitchLog(service_name_, key, from_status, to_status, reason);
    record_info.status.store(to_status);
    // Reset the statistical data of the time window when switch to close state
    record_info.metrics_data.ClearMetrics();
    SetStatusChanged(true);
    return;
  }
}

bool DefaultCircuitBreaker::ShouldChangeToHalfOpen(uint64_t current_ms) {
  std::unique_lock<std::mutex> wl(switch_to_open_times_mutex_);
  if (switch_to_open_times_.empty()) {
    return false;
  }

  if (switch_to_open_times_.front() + config_.sleep_window_ms > current_ms) {
    return false;
  }

  switch_to_open_times_.pop_front();
  wl.unlock();

  // When the sleep time window is reached, switch the nodes in the open state to the half-open state.
  bool flag = false;
  std::shared_lock rl(mutex_);
  for (auto& item : record_info_) {
    auto& record_info = *item.second;
    auto status = record_info.status.load(std::memory_order_relaxed);

    if (status == CircuitBreakStatus::kOpen &&
        record_info.circuitbreak_open_time + config_.sleep_window_ms <= current_ms) {
      if (record_info.status.compare_exchange_strong(status, CircuitBreakStatus::kHalfOpen,
                                                     std::memory_order_relaxed)) {
        record_info.request_count_after_half_open.store(0);
        record_info.success_count_after_half_open.store(0);
        PrintSwitchLog(service_name_, item.first, status, CircuitBreakStatus::kHalfOpen, kReachSleepWindow);
        flag = true;
      }
    }
  }

  return flag;
}

void DefaultCircuitBreaker::AddRecordData(const CircuitBreakRecordKey& key, uint64_t current_ms, bool success) {
  if (!config_.enable) {
    return;
  }

  std::shared_lock<std::shared_mutex> rl(mutex_);
  auto iter = record_info_.find(key);
  if (iter != record_info_.end()) {
    auto& record_info = *(iter->second);
    record_info.metrics_data.AddMetrics(current_ms, success);
    auto status = record_info.status.load(std::memory_order_relaxed);
    if (status == CircuitBreakStatus::kClose) {
      if (!success) {
        auto continuous_error_count = record_info.continuous_error_count.fetch_add(1, std::memory_order_relaxed);
        if (continuous_error_count >= config_.continuous_error_threshold) {
          // When the consecutive failure count reaches the threshold, switch to open state.
          SwitchStatus(key, record_info, current_ms, status, CircuitBreakStatus::kOpen, kContinuousError);
          return;
        }
      } else {
        record_info.continuous_error_count.store(0, std::memory_order_relaxed);
      }

      // Check if the failure rate meets the standard periodically
      auto current_check_error_rate_time = record_info.current_check_error_rate_time.load(std::memory_order_relaxed);
      if (current_check_error_rate_time == 0) {
        record_info.current_check_error_rate_time.store(current_ms, std::memory_order_relaxed);
        current_check_error_rate_time = current_ms;
      }
      if (current_ms - current_check_error_rate_time >= config_.stat_window_ms) {
        // set the next statistical time to stat_window_ms_per_bucket_ later
        record_info.current_check_error_rate_time.store(
            current_ms - config_.stat_window_ms + stat_window_ms_per_bucket_, std::memory_order_relaxed);
        if (record_info.metrics_data.GetErrorRate(current_ms, config_.request_volume_threshold) >=
            config_.error_rate_threshold) {
          SwitchStatus(key, record_info, current_ms, status, CircuitBreakStatus::kOpen, kExcessiveErrorRate);
          return;
        }
      }

    } else if (status == CircuitBreakStatus::kHalfOpen) {
      if (success) {
        record_info.success_count_after_half_open.fetch_add(1, std::memory_order_acq_rel);
      }
      auto request_count_after_half_open =
          record_info.request_count_after_half_open.fetch_add(1, std::memory_order_acq_rel);
      if (request_count_after_half_open == config_.request_count_after_half_open - 1) {
        if (record_info.success_count_after_half_open.load(std::memory_order_acquire) >=
            config_.success_count_after_half_open) {
          SwitchStatus(key, record_info, current_ms, status, CircuitBreakStatus::kClose, kEnoughSuccessCount);
        } else {
          SwitchStatus(key, record_info, current_ms, status, CircuitBreakStatus::kOpen, kNoEnoughSuccessCount);
        }
      }
    }
  }
}

std::unordered_map<CircuitBreakRecordKey, CircuitBreakStatus, CircuitBreakRecordKeyHash>
DefaultCircuitBreaker::GetAllStatus() {
  // copy data
  std::unordered_map<CircuitBreakRecordKey, CircuitBreakStatus, CircuitBreakRecordKeyHash> status;
  std::shared_lock rl(mutex_);
  for (auto& record : record_info_) {
    status.emplace(record.first, record.second->status.load(std::memory_order_relaxed));
  }
  return status;
}

}  // namespace trpc::naming