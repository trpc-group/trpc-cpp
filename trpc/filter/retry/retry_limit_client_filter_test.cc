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

#include "trpc/filter/retry/retry_limit_client_filter.h"

#include "gtest/gtest.h"

namespace trpc::testing {

class RetryLimitClientFilterFixtureTest : public ::testing::Test {
 protected:
  RetryLimitClientFilter filter_{nullptr};
};

TEST_F(RetryLimitClientFilterFixtureTest, GetFilterPoint) {
  std::vector<FilterPoint> points = filter_.GetFilterPoint();
  ASSERT_EQ(points.size(), 2);
  ASSERT_EQ(points[0], FilterPoint::CLIENT_PRE_RPC_INVOKE);
}

TEST_F(RetryLimitClientFilterFixtureTest, Create) {
  std::any param;
  ASSERT_TRUE(filter_.Create(param) != nullptr);

  RetryHedgingLimitConfig config;
  ASSERT_TRUE(filter_.Create(config) != nullptr);
}

TEST_F(RetryLimitClientFilterFixtureTest, operator) {
  RetryHedgingLimitConfig config;
  config.max_tokens = 10;
  config.token_ratio = 2;
  auto filter = filter_.Create(config);

  FilterStatus status = FilterStatus::CONTINUE;
  // Report three consecutive failed calls, causing the token count to drop below half of max_tokens.
  for (int i = 0; i < 3; i++) {
    auto client_context = MakeRefCounted<ClientContext>();
    client_context->SetBackupRequestDelay(10);
    client_context->SetStatus(Status(-1, "request fail"));
    filter->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, client_context);
    ASSERT_TRUE(client_context->IsBackupRequest());

    filter->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, client_context);
  }

  // Check if the failure rate exceeds the limit, the retries are cancelled.
  // Additionally, we report two consecutive successful calls.
  for (int i = 0; i < 2; i++) {
    auto client_context = MakeRefCounted<ClientContext>();
    client_context->SetBackupRequestDelay(10);
    client_context->SetStatus(kSuccStatus);
    filter->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, client_context);
    ASSERT_FALSE(client_context->IsBackupRequest());

    filter->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, client_context);
  }

  // When the success rate improves, retries will be re-enabled.
  auto client_context = MakeRefCounted<ClientContext>();
  client_context->SetBackupRequestDelay(10);
  filter->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, client_context);
  ASSERT_TRUE(client_context->IsBackupRequest());
}

}  // namespace trpc::testing
