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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace trpc::naming {

enum CircuitBreakStatus {
  kClose,
  kOpen,
  kHalfOpen,
};

struct CircuitBreakRecordKey {
  CircuitBreakRecordKey(const std::string& ip_, uint32_t port_) : ip(ip_), port(port_) {}
  std::string ip;
  uint32_t port;
  bool operator==(const CircuitBreakRecordKey& key) const { return (ip == key.ip) && (port == key.port); }
};

struct CircuitBreakRecordKeyHash {
  std::size_t operator()(const CircuitBreakRecordKey& key) const {
    return std::hash<std::string>()(key.ip) ^ std::hash<uint32_t>()(key.port);
  }
};

class CircuitBreaker {
 public:
  /// @brief Reserve only the circuit breaker statistics information of recordkey information passed in the parameters.
  /// @note Call it during initialization or when there is a change in the node set.
  virtual void Reserve(const std::unordered_set<CircuitBreakRecordKey, CircuitBreakRecordKeyHash>& keys) = 0;

  /// @brief Indicate whether there has been a change in the node status.
  /// @note Call it in 'Select' and 'SelectBatch' interface.
  ///       If status changed, it is necessary to update the available node information.
  virtual bool StatusChanged(uint64_t current_ms) = 0;

  /// @brief Add statistical data and switch node status based on invocation conditions.
  /// @note Call it in 'ReportInvokeResult' interface.
  virtual void AddRecordData(const CircuitBreakRecordKey& key, uint64_t current_ms, bool success) = 0;

  /// @brief Retrieve the circuit breaker status of all nodes.
  /// @note Used in conjunction with the 'StatusChanged' interface to update the available node information when their
  /// status changes.
  virtual std::unordered_map<CircuitBreakRecordKey, CircuitBreakStatus, CircuitBreakRecordKeyHash> GetAllStatus() = 0;
};

using CircuitBreakerPtr = std::shared_ptr<CircuitBreaker>;

}  // namespace trpc::naming
