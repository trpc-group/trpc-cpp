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

#include "trpc/overload_control/flow_control/seconds_limiter.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_protocol.h"

namespace trpc::overload_control {

namespace testing {

class SecondsLimiterTest : public ::testing::Test {
 protected:
  void SetUp() override { seconds_limiter_ = std::make_shared<trpc::SecondsLimiter>(3, true); }

  void TearDown() override { seconds_limiter_ = nullptr; }

 protected:
  std::shared_ptr<SecondsLimiter> seconds_limiter_;
};

TEST(SecondsLimiter, Contruct) {
  SecondsLimiter second_limiter(100, true, 0);
  SecondsLimiter second_limiter1(100, false, -1);
  SecondsLimiter second_limiter2(100, true, 100);
  ASSERT_TRUE(true);

  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ASSERT_EQ(second_limiter1.CheckLimit(context), false);
}

TEST_F(SecondsLimiterTest, CheckLimit) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  context->SetRequestMsg(std::move(req_msg));
  context->SetFuncName("trpc.test.flow_control.seconds_limiter");
  ASSERT_EQ(seconds_limiter_->GetMaxCounter(), 3);
  ASSERT_EQ(seconds_limiter_->CheckLimit(context), false);
  ASSERT_EQ(seconds_limiter_->CheckLimit(context), false);
  ASSERT_EQ(seconds_limiter_->GetCurrCounter(), 2);
  ASSERT_EQ(seconds_limiter_->CheckLimit(context), false);
  ASSERT_EQ(seconds_limiter_->CheckLimit(context), true);
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
