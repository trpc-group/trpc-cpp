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

#include "trpc/overload_control/throttler/throttler_client_filter.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/client_filter_manager.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"

namespace trpc::overload_control {
namespace testing {

class ThrottlerClientFilterTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/throttler/throttler.yaml");
  }
  static void TearDownTestCase() {}

  ClientContextPtr MakeClientContext() {
    ClientContextPtr context = MakeRefCounted<ClientContext>();
    context->SetRequest(std::make_shared<trpc::testing::TestProtocol>());
    context->SetFuncName("trpc.test");
    context->SetCallerName("trpc.test.helloworld.Greeter");
    return context;
  }

  bool InitThrottlerClientFilter() {
    MessageClientFilterPtr fiber_limiter_filter(new ThrottlerClientFilter());
    fiber_limiter_filter->Init();
    return ClientFilterManager::GetInstance()->AddMessageClientFilter(fiber_limiter_filter);
  }
};

TEST_F(ThrottlerClientFilterTestFixture, Init) {
  bool ret = InitThrottlerClientFilter();
  ASSERT_TRUE(ret);

  MessageClientFilterPtr filter = ClientFilterManager::GetInstance()->GetMessageClientFilter(kThrottlerName);
  ASSERT_NE(filter, nullptr);
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  ASSERT_EQ(points.size(), 4);
  ASSERT_EQ(filter->Name(), "throttler");
  ASSERT_EQ(filter->GetFilterPoint(), (std::vector<FilterPoint>{
                                          FilterPoint::CLIENT_PRE_RPC_INVOKE,
                                          FilterPoint::CLIENT_POST_RPC_INVOKE,
                                          FilterPoint::CLIENT_PRE_SEND_MSG,
                                          FilterPoint::CLIENT_POST_RECV_MSG,
                                      }));
}

TEST_F(ThrottlerClientFilterTestFixture, OnRequest) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();
  MessageClientFilterPtr filter = ClientFilterManager::GetInstance()->GetMessageClientFilter(kThrottlerName);
  ASSERT_NE(filter, nullptr);

  ClientContextPtr context = MakeClientContext();
  FilterStatus status = FilterStatus::CONTINUE;
  // Dirty request testing.
  context->SetStatus(Status(-1, ""));
  filter->operator()(status, FilterPoint::CLIENT_PRE_SEND_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  // Set IP to empty.
  context->SetAddr("", 0);
  context->SetStatus(Status(0, ""));
  filter->operator()(status, FilterPoint::CLIENT_PRE_SEND_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  context->SetAddr("1.1.1.1", 11112);
  context->SetStatus(Status(0, ""));
  filter->operator()(status, FilterPoint::CLIENT_PRE_SEND_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

TEST_F(ThrottlerClientFilterTestFixture, OnResponse) {
  MessageClientFilterPtr filter = ClientFilterManager::GetInstance()->GetMessageClientFilter(kThrottlerName);
  ASSERT_NE(filter, nullptr);

  ClientContextPtr context = MakeClientContext();
  context->SetStatus(Status(0, ""));
  FilterStatus status = FilterStatus::CONTINUE;

  // Filter data does not exist.
  filter->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  context->SetAddr("1.1.1.1", 11112);
  context->SetStatus(Status(0, ""));
  filter->operator()(status, FilterPoint::CLIENT_PRE_SEND_MSG, context);
  filter->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  filter->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
