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

#include "yaml-cpp/yaml.h"

namespace trpc {

constexpr char kRetryHedgingLimitFilter[] = "retry_hedging_limit";
constexpr int kDefaultRetryHedgingTokensNum = 100;
constexpr int kDefaultRetryHedgingTokenRatio = 10;

/// @brief Thresholds related to the retry hedging and rate limiting strategy.
struct RetryHedgingLimitConfig {
  /// Maximum token value
  int max_tokens{kDefaultRetryHedgingTokensNum};
  /// The ratio between the number of tokens deducted for each failed request and the number of tokens added for each
  /// successful request (which adds 1 token per successful request), also known as the penalty factor.
  int token_ratio{kDefaultRetryHedgingTokenRatio};

  void Display() const;
};

}  // namespace trpc
