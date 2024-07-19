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

#include <stdint.h>

namespace trpc::overload_control {

/// @brief Configuration fields for overload limiter.
constexpr char kOverloadCtrConfField[] = "overload_control";

/// @brief Prometheus monitoring plugin name.
constexpr char kPrometheusMetricsPlugin[] = "prometheus";

/// @brief The indication of successful execution of overload protection, used for monitoring.
constexpr char kOverloadctrlPass[] = "Pass";

/// @brief The indication of interception after overload protection execution, used for monitoring.
constexpr char kOverloadctrlLimited[] = "Limited";

/// @brief The indication of interception due to low priority after overload protection execution, used for monitoring.
constexpr char kOverloadctrlLimitedByLowerPriority[] = "LimitedByLower";

/// @brief Name of overload protection limiter based on request concurrency.
constexpr char kConcurrencyLimiterName[] = "concurrency_limiter";

/// @brief Name of monitoring dimensions for request-based concurrent overload protection rate limiter.
constexpr char kOverloadctrlConcurrencyLimiter[] = "overloadctrl_concurrency_limiter";

/// @brief Name of overload protection limiter based on fiber count.
constexpr char kFiberLimiterName[] = "fiber_limiter";

/// @brief Name of monitoring dimensions for fiber-based overload protection rate limiter.
constexpr char kOverloadctrlFiberLimiter[] = "overloadctrl_fiber_limiter";

/// @brief Flow control name.
constexpr char kFlowControlName[] = "flow_control";

/// @brief Name of monitoring for priority degree in priority algorithm.
constexpr char kOverloadctrlPriority[] = "overloadctrl_priority";

/// @brief Name of high percentile overload protection.
constexpr char kHighPercentileName[] = "high_percentile";

/// @brief Maximum scheduling delay strategy.
constexpr char kExpectedMaxScheduleDelay[] = "max_schedule_delay";

/// @brief Maximum request delay strategy.
constexpr char kExpectedMaxRequestDelay[] = "max_request_latency";

/// @brief Name of monitoring dimensions for high percentile.
constexpr char kOverloadctrlHighPercentile[] = "overloadctrl_high_percentile";

/// @brief Name of monitoring for request latency in high percentile algorithm.
constexpr char kOverloadctrlHighPercentileCost[] = "overloadctrl_high_percentile_cost";

/// @brief Name of monitoring for concurrency degree in high percentile algorithm.
constexpr char kOverloadctrlConcurrency[] = "overloadctrl_high_percentile_concurrency";

/// @brief Client throttler overload protection filter.
constexpr char kThrottlerName[] = "throttler";

/// @brief Name of monitoring dimensions for client throttler.
constexpr char kOverloadctrlThrottler[] = "overloadctrl_throttler";

/// @brief Key for request priority in trpc framework.
constexpr char kTransinfoKeyTrpcPriority[] = "trpc-priority";

/// @brief Name of overload protection limiter based on token bucket.
constexpr char kTokenBucketLimiterName[] = "token_bucket_limiter";

/// @brief Name of monitoring dimensions for request-based concurrent overload protection rate limiter.
constexpr char kOverloadctrlTokenBucketLimiter[] = "overloadctrl_token_bucket_limiter";

}  // namespace trpc::overload_control

#endif
