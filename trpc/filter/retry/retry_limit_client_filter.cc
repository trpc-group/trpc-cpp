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

#include "trpc/filter/retry/retry_limit_client_filter.h"

#include <memory>

#include "trpc/util/log/logging.h"

namespace trpc {

RetryLimitClientFilter::RetryLimitClientFilter(const RetryHedgingLimitConfig* config) {
  if (config != nullptr) {
    TRPC_ASSERT(config->max_tokens > 0 && config->max_tokens < INT32_MAX);
    TRPC_ASSERT(config->token_ratio > 0 && config->token_ratio <= config->max_tokens);
    config_ = *config;
    tokens_num_ = config_.max_tokens;
    TRPC_FMT_DEBUG("Set tokens_num_ = {}", tokens_num_);
  } else {
    tokens_num_ = config_.max_tokens;
  }
}

std::vector<FilterPoint> RetryLimitClientFilter::GetFilterPoint() {
  std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  return points;
}

void RetryLimitClientFilter::operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) {
  switch (point) {
    case FilterPoint::CLIENT_PRE_RPC_INVOKE:
      // The decision to enable retries is based on the number of tokens: if the number of tokens is less than or equal
      // to half of the capacity, the retry strategy will be aborted.
      if (tokens_num_ <= (config_.max_tokens >> 1)) {
        if (context->IsBackupRequest()) {
          TRPC_FMT_WARN("Cancel retry due to high failure rate, tokens num = {}", tokens_num_);
          context->CancelBackupRequest();
        }
      }
      break;
    case FilterPoint::CLIENT_POST_RPC_INVOKE:
      // Update the token count based on the call result: one token is added for a successful call, and token_ratio
      // tokens are deducted for a failed call.
      if (context->GetStatus().OK()) {
        if (tokens_num_.fetch_add(1, std::memory_order_acq_rel) >= config_.max_tokens) {
          tokens_num_.fetch_sub(1, std::memory_order_acq_rel);
        }
      } else {
        if (tokens_num_.fetch_sub(config_.token_ratio, std::memory_order_acq_rel) < 0) {
          tokens_num_.store(0, std::memory_order_release);
        }
      }
      break;
    default:
      break;
  }
}

MessageClientFilterPtr RetryLimitClientFilter::Create(const std::any& param) {
  RetryHedgingLimitConfig config;
  if (param.has_value()) {
    config = std::any_cast<RetryHedgingLimitConfig>(param);
  }

  return std::make_shared<RetryLimitClientFilter>(&config);
}

}  // namespace trpc
