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

#include "gtest/gtest.h"

#include "trpc/util/time.h"

namespace trpc::naming::testing {

class DefaultCircuitBreakerTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {}

  static void TearDownTestCase() {}

  virtual void SetUp() {
    config_.enable = true;
    circuit_breaker_ = std::make_shared<DefaultCircuitBreaker>(config_, "");
  }

  virtual void TearDown() { circuit_breaker_.reset(); }

 protected:
  CircuitBreakerPtr circuit_breaker_{nullptr};
  DefaultCircuitBreakerConfig config_;
};

TEST_F(DefaultCircuitBreakerTest, Reserve) {
  // After calling Reserve during initialization, the recorddata can be correspondingly updated
  std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash> keys;
  keys.emplace(CircuitBreakRecordKey("127.0.0.1", 10001));
  keys.emplace(CircuitBreakRecordKey("127.0.0.1", 10002));
  keys.emplace(CircuitBreakRecordKey("127.0.0.1", 10003));
  circuit_breaker_->Reserve(keys);
  auto all_status = circuit_breaker_->GetAllStatus();
  ASSERT_TRUE(all_status.size() == keys.size());
  for (auto& key : keys) {
    ASSERT_TRUE(all_status.find(key) != all_status.end());
  }

  // Calling Reserve after adding or removing nodes, the recorddata will also reflect the corresponding changes
  keys.emplace(CircuitBreakRecordKey("127.0.0.1", 10004));
  keys.erase(CircuitBreakRecordKey("127.0.0.1", 10003));
  circuit_breaker_->Reserve(keys);
  all_status = circuit_breaker_->GetAllStatus();
  ASSERT_TRUE(all_status.size() == keys.size());
  for (auto& key : keys) {
    ASSERT_TRUE(all_status.find(key) != all_status.end());
  }
}

// Test when the consecutive failure count meets the criteria, the node will trigger a circuit breaker.
// It will switch from open state to the half-open state after the sleep_window time has elapsed.
// If the success count meets the criteria during this period, it will switch to the closed state.
// If the success count does not meet the criteria during this period, it will switch to the open state.
TEST_F(DefaultCircuitBreakerTest, ContinuousFailAndResume) {
  CircuitBreakRecordKey key1("127.0.0.1", 10001);
  CircuitBreakRecordKey key2("127.0.0.1", 10002);
  // Initialize
  std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash> keys;
  keys.emplace(CircuitBreakRecordKey(key1));
  keys.emplace(CircuitBreakRecordKey(key2));
  circuit_breaker_->Reserve(keys);

  // trigger a circuit break when the consecutive failure count meets the criteria
  for (uint32_t i = 1; i <= config_.continuous_error_threshold; i++) {
    for (auto& key : keys) {
      uint64_t current_ms = GetMilliSeconds();
      circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key), current_ms, false);
      auto all_status = circuit_breaker_->GetAllStatus();
      if (i < config_.continuous_error_threshold) {
        ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
        ASSERT_TRUE(all_status.find(key)->second == CircuitBreakStatus::kClose);
      } else {
        ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
        ASSERT_TRUE(all_status.find(key)->second == CircuitBreakStatus::kOpen);
      }
    }
  }

  uint64_t current_ms = GetMilliSeconds();
  ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));

  // switch from open state to half-open state after the sleep_window time has elapsed
  current_ms += config_.sleep_window_ms;
  ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
  auto all_status = circuit_breaker_->GetAllStatus();
  for (auto& key : keys) {
    ASSERT_TRUE(all_status.find(key)->second == CircuitBreakStatus::kHalfOpen);
  }

  // switch from half-open state to closed state when the success count meets the criteria during this period
  for (uint32_t i = 1; i <= config_.request_count_after_half_open; i++) {
    circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key1), current_ms, true);
    auto all_status = circuit_breaker_->GetAllStatus();
    if (i < config_.request_count_after_half_open) {
      ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key1)->second == CircuitBreakStatus::kHalfOpen);
    } else {
      ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key1)->second == CircuitBreakStatus::kClose);
    }
  }

  // switch from half-open state to open state when the success count does not meet the criteria during this period
  for (uint32_t i = 1; i <= config_.request_count_after_half_open; i++) {
    if (i % 2 == 0) {
      circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key2), current_ms, true);
    } else {
      circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key2), current_ms, false);
    }

    auto all_status = circuit_breaker_->GetAllStatus();
    if (i < config_.request_count_after_half_open) {
      ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key2)->second == CircuitBreakStatus::kHalfOpen);
    } else {
      ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key2)->second == CircuitBreakStatus::kOpen);
    }
  }
}

// Test when the failure rate meets the criteria, the node will trigger a circuit breaker.
TEST_F(DefaultCircuitBreakerTest, ReachErrorRateAndResume) {
  CircuitBreakRecordKey key1("127.0.0.1", 10001);
  CircuitBreakRecordKey key2("127.0.0.1", 10002);
  // Initialize
  std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash> keys;
  keys.emplace(CircuitBreakRecordKey(key1));
  keys.emplace(CircuitBreakRecordKey(key2));
  circuit_breaker_->Reserve(keys);

  uint64_t interval = config_.stat_window_ms / config_.buckets_num;
  for (uint32_t i = 0; i <= config_.buckets_num; i++) {
    for (auto& key : keys) {
      uint64_t current_ms = GetMilliSeconds() - config_.stat_window_ms + i * interval;
      if (i % 3 != 0) {
        circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key), current_ms, false);
      } else {
        circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key), current_ms, true);
      }
      auto all_status = circuit_breaker_->GetAllStatus();
      if (i < config_.buckets_num) {
        ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
        ASSERT_TRUE(all_status.find(key)->second == CircuitBreakStatus::kClose);
      } else {
        ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
        ASSERT_TRUE(all_status.find(key)->second == CircuitBreakStatus::kOpen);
      }
    }
  }

  uint64_t current_ms = GetMilliSeconds();
  ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));

  // switch from open state to the half-open state after the sleep_window time has elapsed
  current_ms += config_.sleep_window_ms;
  ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
  auto all_status = circuit_breaker_->GetAllStatus();
  for (auto& key : keys) {
    ASSERT_TRUE(all_status.find(key)->second == CircuitBreakStatus::kHalfOpen);
  }

  // switch from half-open state to closed state when the success count meets the criteria during this period
  for (uint32_t i = 1; i <= config_.request_count_after_half_open; i++) {
    circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key1), current_ms, true);
    auto all_status = circuit_breaker_->GetAllStatus();
    if (i < config_.request_count_after_half_open) {
      ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key1)->second == CircuitBreakStatus::kHalfOpen);
    } else {
      ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key1)->second == CircuitBreakStatus::kClose);
    }
  }

  // switch from half-open state to open state when the success count does not meet the criteria during this period
  for (uint32_t i = 1; i <= config_.request_count_after_half_open; i++) {
    if (i % 2 == 0) {
      circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key2), current_ms, true);
    } else {
      circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key2), current_ms, false);
    }

    auto all_status = circuit_breaker_->GetAllStatus();
    if (i < config_.request_count_after_half_open) {
      ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key2)->second == CircuitBreakStatus::kHalfOpen);
    } else {
      ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key2)->second == CircuitBreakStatus::kOpen);
    }
  }
}

// The circuit breaker will not be triggered when the node is called normally
TEST_F(DefaultCircuitBreakerTest, NoTriggerCircuitBreak) {
  CircuitBreakRecordKey key1("127.0.0.1", 10001);
  // Initialize
  std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash> keys;
  keys.emplace(CircuitBreakRecordKey(key1));
  circuit_breaker_->Reserve(keys);

  // Simulate 10 rounds of calls, with a 20% chance of error in each round
  uint64_t interval = config_.stat_window_ms / config_.buckets_num;
  uint32_t loop_times = 10;
  for (uint32_t i = 1; i <= config_.buckets_num * loop_times; i++) {
    uint64_t current_ms = GetMilliSeconds() - config_.stat_window_ms * loop_times + i * interval;
    if (i % 5 != 0) {
      circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key1), current_ms, true);
    } else {
      circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key1), current_ms, false);
    }
    auto all_status = circuit_breaker_->GetAllStatus();
    ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
    ASSERT_TRUE(all_status.find(key1)->second == CircuitBreakStatus::kClose);
  }
}

// If the failure rate meets the criteria within the statistical period but does not reach the threshold for the number
// of requests, the circuit breaker will not be triggered.
// And if the consecutive failure count is greater than or equal to the consecutive failure count threshold, the circuit
// breaker will be triggered.
TEST_F(DefaultCircuitBreakerTest, NoReachRequestVolumeThreshold) {
  CircuitBreakRecordKey key1("127.0.0.1", 10001);
  // Initialize
  std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash> keys;
  keys.emplace(CircuitBreakRecordKey(key1));
  circuit_breaker_->Reserve(keys);

  uint64_t interval = config_.stat_window_ms * 2 / config_.request_volume_threshold;
  for (uint32_t i = 1; i <= config_.request_volume_threshold; i++) {
    uint64_t current_ms = GetMilliSeconds() - config_.stat_window_ms * 2 + i * interval;
    circuit_breaker_->AddRecordData(CircuitBreakRecordKey(key1), current_ms, false);
    auto all_status = circuit_breaker_->GetAllStatus();
    if (i < config_.continuous_error_threshold) {
      ASSERT_FALSE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key1)->second == CircuitBreakStatus::kClose);
    } else if (i == config_.continuous_error_threshold) {
      ASSERT_TRUE(circuit_breaker_->StatusChanged(current_ms));
      ASSERT_TRUE(all_status.find(key1)->second == CircuitBreakStatus::kOpen);
    }
  }
}

}  // namespace trpc::naming::testing