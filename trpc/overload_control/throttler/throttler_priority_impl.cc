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

#include "trpc/overload_control/throttler/throttler_priority_impl.h"

namespace trpc::overload_control {

// Maximum number of tokens that can be borrowed.
static constexpr int64_t kTokenBufferSize = 30;

ThrottlerPriorityImpl::ThrottlerPriorityImpl(const Options& options) : throttler_(options.throttler), available_(0) {}

bool ThrottlerPriorityImpl::MustOnRequest() {
  if (!throttler_->OnRequest()) {
    // For high-priority requests with "MustOnRequest", if they fail to obtain normally, borrow a quota that will be
    // returned by requests with "OnRequest" is from the token.
    available_.fetch_sub(1, std::memory_order_relaxed);
  }

  return true;
}

bool ThrottlerPriorityImpl::OnRequest() {
  if (!throttler_->OnRequest()) {
    return false;
  }

  // First, attempt to repay the outstanding balance of the token pool.
  if (available_.fetch_add(1, std::memory_order_relaxed) < 0) {
    // If the outstanding balance is successfully repaid, then this request will be rejected as a repayment cost.
    return false;
  }

  // If it reaches this point, it means that there is no outstanding balance. Therefore, the repayment will be revoked
  // and the request will continue.
  available_.fetch_sub(1, std::memory_order_relaxed);

  return true;
}

void ThrottlerPriorityImpl::OnSuccess(std::chrono::steady_clock::duration cost) {
  // no-op
}

void ThrottlerPriorityImpl::OnError() {
  // no-op
}

void ThrottlerPriorityImpl::FillToken() {
  if (available_.fetch_add(1) > kTokenBufferSize) {
    available_.fetch_sub(1);
  }
}

}  // namespace trpc::overload_control

#endif
