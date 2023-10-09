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

#include "trpc/overload_control/throttler/throttler_ema.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::overload_control {

/// @brief Interception algorithm based on downstream success rate.
class Throttler {
 public:
  /// @brief Options
  struct Options {
    // Incremental acceptance rate, usually a value slightly greater than 1, indicating that "slight" overload is
    // allowed, based on experience.
    double ratio_for_accepts = 1.3;
    // A denominator padding used to reduce false positives at low QPS when calculating pass rates, based on experience.
    double requests_padding = 8;
    // Maximum rate limit, based on experience.
    double max_throttle_probability = 0.7;
    // EMA options
    ThrottlerEma::Options ema_options;
  };

 public:
  explicit Throttler(const Options& options);

  /// @brief Process requests by algorithm the result of which determine whether this request is allowed.
  /// @return true: pass; false: reject
  bool OnRequest();

  /// @brief Process the response of current request to update algorithm data for the next request processing.
  /// @param throttled Indicates whether the request is a rate-limited request, which can be true in following cases.
  /// @note - OnRequest returned false before the request.
  //        - The downstream request returned error statuses related to rate limiting or overload.
  void OnResponse(bool throttled);

 private:
  // Options
  Options options_;

  // The EMA (exponential moving average) of the average number of successful responses to requests.
  ThrottlerEma accepts_ema_;

  // The EMA (exponential moving average) of the average number of intercepted requests.
  ThrottlerEma throttles_ema_;

  // Mutex lock, in this class, only `try_lock` is used for non-blocking mutual access.
  Spinlock lock_;
};

}  // namespace trpc::overload_control

#endif
