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
#include <memory>
#include <utility>

#include "trpc/tvar/basic_ops/reducer.h"

namespace trpc {

/// @brief Statistical data of server, such as the total number of requests,
/// the number of failed requests, and the average response time.
class ServerStats {
 public:
  /// @brief Increase or decrease the number of connections
  void AddConnCount(int count);

  /// @brief Get the number of connections
  int64_t GetConnCount() const { return conn_count_.GetValue(); }

  /// @brief Increase the number of requests
  void AddReqCount(uint64_t count = 1);

  /// @brief Get the total number of requests
  uint64_t GetTotalReqCount() const { return total_req_count_.GetValue(); }

  /// @brief Get the number of requests in the current statistical period
  uint64_t GetReqCount() const { return req_count_.GetValue(); }

  /// @brief Increase the number of failed requests
  void AddFailedReqCount(uint64_t count = 1);

  /// @brief Get the total number of failed requests
  uint64_t GetTotalFailedReqCount() const { return total_failed_req_count_.GetValue(); }

  /// @brief Get the number of failed requests in the current statistical period
  uint64_t GetFailedReqCount() const { return failed_req_count_.GetValue(); }

  /// @brief Increase request latency time
  void AddReqDelay(uint64_t delay_in_ms);

  /// @brief Get the number of requests in the last statistical period
  uint64_t GetLastReqCount() const { return last_req_count_.load(std::memory_order_relaxed); }

  /// @brief Get the number of failed requests in the last statistical period
  uint64_t GetLastFailedReqCount() const { return last_failed_req_count_.load(std::memory_order_relaxed); }

  /// @brief Get the average request latency time
  double GetAvgTotalDelay() const {
    auto total_req_count = GetTotalReqCount();
    if (total_req_count > 0) {
      return total_req_delay_.GetValue() * 1.0 / total_req_count;
    }
    return 0.0;
  }

  /// @brief Get the average request latency time in the last statistical period
  double GetAvgLastDelay() const {
    auto last_req_count = GetLastReqCount();
    if (last_req_count > 0) {
      return last_req_delay_.load(std::memory_order_relaxed) * 1.0 / last_req_count;
    }
    return 0.0;
  }

  /// @brief Get the average request latency time in the current statistical period
  double GetAvgDelay() const {
    auto req_count = GetReqCount();
    if (req_count > 0) {
      return req_delay_.GetValue() * 1.0 / req_count;
    }
    return 0;
  }

  /// @brief Get the max request latency time in the last statistical period
  uint64_t GetLastMaxDelay() const { return last_max_delay_.load(std::memory_order_relaxed); }

  /// @brief Get the max request latency time in the current statistical period
  uint64_t GetMaxDelay() const { return max_delay_.GetValue(); }

  /// @brief Get the current concurrent request count
  uint32_t GetReqConcurrency() const { return req_concurrency_.load(std::memory_order_relaxed); }

  /// @brief Increase concurrent request count
  uint32_t AddReqConcurrency() { return req_concurrency_.fetch_add(1, std::memory_order_relaxed); }

  /// @brief Decrease concurrent request count
  uint32_t SubReqConcurrency() { return req_concurrency_.fetch_sub(1, std::memory_order_relaxed); }

  /// @brief Print statistical data
  void Stats();

 private:
  // number of connections
  tvar::Gauge<int64_t> conn_count_{0};

  // number of requests
  tvar::Counter<uint64_t> total_req_count_{0};

  // number of requests in the current statistical period
  tvar::Counter<uint64_t> req_count_{0};

  // number of requests in the last statistical period
  std::atomic<uint64_t> last_req_count_{0};

  // total number of failed requests
  tvar::Counter<uint64_t> total_failed_req_count_{0};

  // number of failed requests in the current statistical period
  tvar::Counter<uint64_t> failed_req_count_{0};

  // number of failed requests in the last statistical period
  std::atomic<uint64_t> last_failed_req_count_{0};

  // total request latency time
  tvar::Counter<uint64_t> total_req_delay_{0};

  // request latency time in the current statistical period
  tvar::Counter<uint64_t> req_delay_{0};

  // request latency time in the last statistical period
  std::atomic<uint64_t> last_req_delay_{0};

  // max request latency time in the current statistical period
  tvar::Maxer<uint64_t> max_delay_{0};

  // max request latency time in the last statistical period
  std::atomic<uint64_t> last_max_delay_{0};

  // concurrent request count
  std::atomic<uint32_t> req_concurrency_{0};
};

}  // namespace trpc
