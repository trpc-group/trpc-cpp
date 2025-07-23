//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <atomic>

#include "trpc/overload_control/common/priority.h"
#include "trpc/overload_control/throttler/throttler.h"

namespace trpc::overload_control {

/// @brief Implementation of overload protection interface based on request success rate.
/// @note Just for client-side
class ThrottlerPriorityImpl final : public Priority {
 public:
  /// @brief Options
  struct Options {
    Throttler* throttler = nullptr;  // Throttler instance
  };

 public:
  explicit ThrottlerPriorityImpl(const Options& options);

  /// @brief Process high-priority requests by algorithm the result of which determine whether this request is allowed.
  /// @return true: pass; false: reject
  /// @note This method corresponds to high-priority requests and uses a relatively loose judgment method.
  bool MustOnRequest() override;

  /// @brief Process requests by algorithm the result of which determine whether this request is allowed.
  /// @return true: pass; false: reject
  /// @note This method corresponds to medium-priority requests and uses a normal judgment method.
  bool OnRequest() override;

  /// @brief Update algorithm data by recording the cost time of the request after it is processed successfully
  /// @param cost The cost time of the reques
  void OnSuccess(std::chrono::steady_clock::duration cost) override;

  /// @brief Update algorithm data when an error occurs during request processing.
  void OnError() override;

  /// @brief Return token to pool
  void FillToken();

 private:
  // Throttler instance
  Throttler* throttler_;

  // `available_`: The concept of "Token Pool" in the algorithm.
  // `MustOnRequest()`: It is possible to borrow a token, causing `available_` to become negative, indicating a debt.
  // `OnRequest()`: When `available_` is in a negative state, return a token and intercept the request.
  std::atomic_int64_t available_;
};

}  // namespace trpc::overload_control

#endif
