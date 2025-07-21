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

#include "trpc/overload_control/concurrency_limiter/concurrency_limiter_server_filter.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/runtime/common/stats/frame_stats.h"

namespace trpc::overload_control {
namespace testing {

class ConcurrencyLimiterServerTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/concurrency_limiter/concurrency_overload_ctrl.yaml");
    bool ret = InitConcurrencyLimiterServerFilter();
    ASSERT_TRUE(ret);
  }
  static void TearDownTestCase() {}

  static ServerContextPtr MakeServerContext() {
    auto context = MakeRefCounted<ServerContext>();
    context->SetRequestMsg(std::make_shared<trpc::testing::TestProtocol>());
    return context;
  }

  static bool InitConcurrencyLimiterServerFilter() {
    MessageServerFilterPtr concurrency_limiter_filter(new overload_control::ConcurrencyLimiterServerFilter());
    concurrency_limiter_filter->Init();
    return FilterManager::GetInstance()->AddMessageServerFilter(concurrency_limiter_filter);
  }
};

TEST_F(ConcurrencyLimiterServerTestFixture, Init) {
  MessageServerFilterPtr filter = FilterManager::GetInstance()->GetMessageServerFilter(kConcurrencyLimiterName);
  ASSERT_NE(filter, nullptr);
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  ASSERT_EQ(points.size(), 2);
}

// Test concurrency does not exceed the limit.
TEST_F(ConcurrencyLimiterServerTestFixture, Ok) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kConcurrencyLimiterName);
  ConcurrencyLimiterServerFilter* concurr_filter = static_cast<ConcurrencyLimiterServerFilter*>(filter.get());

  ServerContextPtr context = MakeServerContext();
  FilterStatus status = FilterStatus::CONTINUE;
  concurr_filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  concurr_filter->operator()(status, FilterPoint::SERVER_POST_SCHED_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

// Test concurrency exceeds the limit.
TEST_F(ConcurrencyLimiterServerTestFixture, Overload) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kConcurrencyLimiterName);
  ConcurrencyLimiterServerFilter* concurr_filter = static_cast<ConcurrencyLimiterServerFilter*>(filter.get());

  uint32_t max_concurr = 20;
  for (uint32_t i = 0; i < max_concurr + 1; i++) {
    FrameStats::GetInstance()->GetServerStats().AddReqConcurrency();
  }

  ServerContextPtr context = MakeServerContext();
  FilterStatus status = FilterStatus::CONTINUE;
  concurr_filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), false);
  ASSERT_EQ(status, FilterStatus::REJECT);

  for (uint32_t i = 0; i < max_concurr + 1; i++) {
    FrameStats::GetInstance()->GetServerStats().SubReqConcurrency();
  }
}
}  // namespace testing
}  // namespace trpc::overload_control

#endif
