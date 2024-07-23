//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent公司.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "fixedwindow_limiter_server_filter.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/runtime/common/stats/frame_stats.h"

namespace trpc::overload_control {
namespace testing {

class FixedTimeWindowServerTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/fixedwindow_limiter/fixedwindow_overload_ctrl.yaml");
    bool ret = InitFixedTimeWindowServerFilter();
    ASSERT_TRUE(ret);
  }
  static void TearDownTestCase() {}

  static ServerContextPtr MakeServerContext() {
    auto context = MakeRefCounted<ServerContext>();
    context->SetRequestMsg(std::make_shared<trpc::testing::TestProtocol>());
    return context;
  }

  static bool InitFixedTimeWindowServerFilter() {
    MessageServerFilterPtr fixedwindow_limiter_filter(new overload_control::FixedTimeWindowServerFilter());
    fixedwindow_limiter_filter->Init();
    return FilterManager::GetInstance()->AddMessageServerFilter(fixedwindow_limiter_filter);
  }
};

TEST_F(FixedTimeWindowServerTestFixture, Init) {
  MessageServerFilterPtr filter = FilterManager::GetInstance()->GetMessageServerFilter("fixedwindow_limiter");
  ASSERT_NE(filter, nullptr);
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  ASSERT_EQ(points.size(), 2);
}

// Test to ensure that the number of requests does not exceed the limit, to guarantee that flow control does not occur.
TEST_F(FixedTimeWindowServerTestFixture, Ok) {
  MessageServerFilterPtr filter = FilterManager::GetInstance()->GetMessageServerFilter("fixedwindow_limiter");
  FixedTimeWindowServerFilter* time_filter = static_cast<FixedTimeWindowServerFilter*>(filter.get());
  ServerContextPtr context = MakeServerContext();
  FilterStatus status = FilterStatus::CONTINUE;
  time_filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  time_filter->operator()(status, FilterPoint::SERVER_POST_SCHED_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  
}

// Test to ensure that the number of requests exceeds the limit, causing flow control to occur.
TEST_F(FixedTimeWindowServerTestFixture, Overload) {
  MessageServerFilterPtr filter = FilterManager::GetInstance()->GetMessageServerFilter("fixedwindow_limiter");
  FixedTimeWindowServerFilter* time_filter = static_cast<FixedTimeWindowServerFilter*>(filter.get());
  uint32_t request_count = 25;
  uint32_t rejected_count = 0;
  for (uint32_t i = 0; i < request_count; i++) {
    ServerContextPtr context = MakeServerContext();
    FilterStatus status = FilterStatus::CONTINUE;
    time_filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
    if(!context->GetStatus().OK())
       rejected_count++;
     // verify whether the time window reset logic takes effect
    //std::this_thread::sleep_for(std::chrono::seconds(1));
        }
  ASSERT_TRUE(rejected_count>=10);

  }
}  // namespace testing
}  // namespace trpc::overload_control

#endif
