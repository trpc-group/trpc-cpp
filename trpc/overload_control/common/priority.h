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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <chrono>

namespace trpc::overload_control {

/// @brief Abstract interface for overload protection algorithm.
/// After the implementation of this interface is adapted, it can support input with continuous priority.
class Priority {
 public:
  virtual ~Priority() = default;

  /// @brief Process high-priority requests by algorithm the result of which determine whether this request is allowed.
  /// @return bool type,true: pass；false: reject
  /// @note This method call corresponds to a high-priority request and uses a relatively loose judgment method.
  virtual bool MustOnRequest() = 0;

  /// @brief Process normal requests by algorithm the result of which determine whether this request is allowed.
  /// @return true: pass；false: reject
  /// @note This method call corresponds to a medium-priority request and uses a normal judgment method.
  virtual bool OnRequest() = 0;

  /// @brief Update algorithm data by recording the cost time of the request after it is processed successfully
  /// @param cost The parameter provides the final time cost of this request.
  virtual void OnSuccess(std::chrono::steady_clock::duration cost) = 0;

  /// @brief Update algorithm data when an error occurs during request processing.
  virtual void OnError() = 0;
};

}  // namespace trpc::overload_control

#endif
