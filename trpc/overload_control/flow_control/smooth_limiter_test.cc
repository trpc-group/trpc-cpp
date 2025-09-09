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

#include "trpc/overload_control/flow_control/smooth_limiter.h"

#include <chrono>
#include <memory>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_protocol.h"

namespace trpc::overload_control {

namespace testing {

class SmoothLimiterTest : public ::testing::Test {
 protected:
  void SetUp() override { smooth_limiter_ = std::make_shared<trpc::SmoothLimiter>(3, true); }

  void TearDown() override { smooth_limiter_ = nullptr; }

 protected:
  std::shared_ptr<SmoothLimiter> smooth_limiter_;
};

TEST(SmoothLimiter, Contruct) {
  SmoothLimiter smooth_limiter(100, true, 0);
  SmoothLimiter smooth_limiter1(100, false, -1);
  SmoothLimiter smooth_limiter2(100, true, 100);
  ASSERT_TRUE(true);

  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ASSERT_EQ(smooth_limiter1.CheckLimit(context), false);
}

TEST_F(SmoothLimiterTest, CheckLimit) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  context->SetRequestMsg(std::move(req_msg));
  context->SetFuncName("trpc.test.flow_control.smooth_limiter");

  ASSERT_EQ(smooth_limiter_->GetMaxCounter(), 3);
  ASSERT_EQ(smooth_limiter_->CheckLimit(context), false);
  ASSERT_EQ(smooth_limiter_->CheckLimit(context), false);
  ASSERT_EQ(smooth_limiter_->GetCurrCounter(), 2);
  ASSERT_EQ(smooth_limiter_->CheckLimit(context), false);
  ASSERT_EQ(smooth_limiter_->CheckLimit(context), true);

  std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST_F(SmoothLimiterTest, Overload) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  context->SetRequestMsg(std::move(req_msg));
  context->SetFuncName("trpc.test.flow_control.smooth_limiter");

  ASSERT_EQ(smooth_limiter_->GetMaxCounter(), 3);
  for (int i{0}; i < 4; ++i) {
    ASSERT_EQ(smooth_limiter_->GetCurrCounter(), 0);
    ASSERT_EQ(smooth_limiter_->CheckLimit(context), false);
    ASSERT_EQ(smooth_limiter_->CheckLimit(context), false);
    ASSERT_EQ(smooth_limiter_->GetCurrCounter(), 2);
    ASSERT_EQ(smooth_limiter_->CheckLimit(context), false);
    ASSERT_EQ(smooth_limiter_->CheckLimit(context), true);
    ASSERT_EQ(smooth_limiter_->CheckLimit(context), true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
