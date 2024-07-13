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

#include "trpc/filter/filter.h"
#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_conf.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/server/server_context.h"

namespace trpc::overload_control {

/// @brief Server-side token bucket limiting class.
/// @note Mainly limit the concurrency of requests by token bucket algorithm.
class TokenBucketLimiterServerFilter : public MessageServerFilter {
public:
  /// @brief Name of filter
  std::string Name() override { return kTokenBucketLimiterName; }

  /// @brief Initialization function.
  int Init() override;

  /// @brief Get the collection of tracking points
  std::vector<FilterPoint> GetFilterPoint() override;

  /// @brief Execute the logic corresponding to the tracking point.
  void operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) override;

private:
  // Process requests by algorithm the result of which determine whether this request is allowed.
  void OnRequest(FilterStatus& status, const ServerContextPtr& context);

private:
  TokenBucketLimiterControlConf token_bucket_conf_;
};

}  // namespace trpc::overload_control

#endif
