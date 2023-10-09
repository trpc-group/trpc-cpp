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

#include "trpc/client/client_context.h"
#include "trpc/common/config/retry_conf.h"
#include "trpc/filter/client_filter_base.h"

namespace trpc {

/// @brief The rate limiting protection filter that used in conjunction with the backup request retry feature. It is
///        used to disable  retries when the request failure rate is too high, in order to avoid overloading the backend
///        service.
///        The algorithm is implemented using a token bucket mechanism:
///        1. The number of tokens in the bucket is initialized to max_tokens. When a request succeeds, the token count
///        is increased by 1 until it reaches max_tokens.
///        2. When a request fails, the token count is decreased by N (i.e. token_ratio) until it is less than 0.
///        3. The retry/hedging strategy only works normally when the number of tokens in the bucket is greater than
///        half of the capacity. When the number of tokens is less than or equal to half of the capacity, retries are
///        cancelled.
/// @note If either the original request or the retry request succeeds, it is considered a success. If both fail,
///       it is considered a failure.
class RetryLimitClientFilter : public MessageClientFilter {
 public:
  explicit RetryLimitClientFilter(const RetryHedgingLimitConfig* config);

  std::string Name() override { return kRetryHedgingLimitFilter; }

  std::vector<FilterPoint> GetFilterPoint() override;

  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override;

  MessageClientFilterPtr Create(const std::any& param) override;

 private:
  // Threshold configuration for retry strategy
  RetryHedgingLimitConfig config_;

  // The number of tokens in the token bucket for the token bucket algorithm
  std::atomic<int> tokens_num_{0};
};

}  // namespace trpc
