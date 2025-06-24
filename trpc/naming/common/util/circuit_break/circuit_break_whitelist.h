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

#include <set>
#include <vector>

namespace trpc::naming {

namespace detail {
template <typename T>
class ReadersWriterData {
 public:
  virtual const T& Reader() { return instances_[reader_]; }

  virtual T& Writer() { return instances_[writer_]; }

  virtual void Swap() { std::swap(reader_, writer_); }

 public:
  ReadersWriterData() : reader_(0), writer_(1) {}
  virtual ~ReadersWriterData() = default;

 private:
  T instances_[2];
  int reader_{0};
  int writer_{1};
};
}  // namespace detail

/// @brief Class of circuit breaker error code whitelist, is not thread safe.
class CircuitBreakWhiteList {
 public:
  CircuitBreakWhiteList();

  /// @brief Set error code of whitelist
  void SetCircuitBreakWhiteList(const std::vector<int>& retcodes);

  /// @brief Determine whether the error code is in the whitelist
  /// @return Return true if the error code is in the whitelist, otherwise return false
  bool Contains(int retcode);

 private:
  detail::ReadersWriterData<std::set<int>> circuitbreak_whitelist_;
};

}  // namespace trpc::naming
