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

#include <atomic>
#include <chrono>
#include <memory>

#include "trpc/client/client_context.h"
#include "trpc/overload_control/common/priority_adapter.h"
#include "trpc/overload_control/throttler/throttler.h"
#include "trpc/overload_control/throttler/throttler_priority_impl.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::overload_control {

/// @brief Throttling overload protection controller.
class ThrottlerOverloadController : public RefCounted<ThrottlerOverloadController> {
 public:
  /// @brief Options
  struct Options {
    std::string ip_port;                                ///< The client reports the IP and port.
    bool is_report;                                     ///< Whether to report result to metric plugin
    PriorityAdapter::Options priority_adapter_options;  ///< PriorityAdapter options
    Throttler::Options throttler_options;               ///< Throttler options
  };

 public:
  explicit ThrottlerOverloadController(const Options& options);

  /// @brief Process requests by algorithm the result of which determine whether this request is allowed.
  /// @param context Client context
  /// @return true: pass; false: reject
  bool OnRequest(const ClientContextPtr& context);

  /// @brief Process the response of current request to update algorithm data for the next request processing.
  /// @param context Client context
  void OnResponse(const ClientContextPtr& context);

  /// @brief Check if the controller has expired due to long periods of inactivity.
  /// @return true: expired；false: not expired
  bool IsExpired();

  /// @brief Check if the downstream error code belongs to the overload or rate limiting category.
  /// @param status Status of processing request
  /// @return true: overload or rate limiting error code; false: other error code
  static bool IsOverloadException(const Status& status);

 private:
  // Options
  Options options_;

  // Throttler
  std::unique_ptr<Throttler> throttler_;

  // ThrottlerPriorityImpl instance
  std::unique_ptr<ThrottlerPriorityImpl> throttler_priority_impl_;

  // Priority adapter instance.
  std::unique_ptr<PriorityAdapter> priority_adapter_;

  // Last operation time, used to determine if it has expired and can be cleared.
  std::atomic<std::chrono::steady_clock::time_point> time_point_;
};

using ThrottlerOverloadControllerPtr = RefPtr<ThrottlerOverloadController>;
}  // namespace trpc::overload_control

#endif
