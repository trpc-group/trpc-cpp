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

#include "trpc/overload_control/fiber_limiter/fiber_limiter_client_filter.h"

#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/transport/client/fiber/testing/thread_model_op.h"

namespace trpc::overload_control {
namespace testing {

class FiberLimiterClientTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::TrpcPlugin::GetInstance()->RegisterPlugins();
    InitFiberLimiterClientFilter();
  }
  static void TearDownTestCase() { trpc::TrpcPlugin::GetInstance()->UnregisterPlugins(); }

  static ClientContextPtr MakeClientContext() {
    auto context = MakeRefCounted<ClientContext>();
    context->SetRequest(std::make_shared<trpc::testing::TestProtocol>());
    context->SetFuncName("trpc.test");
    context->SetCallerName("trpc.test.helloworld.Greeter");
    return context;
  }

  static bool InitFiberLimiterClientFilter() {
    MessageClientFilterPtr fiber_limiter_filter(new overload_control::FiberLimiterClientFilter());
    fiber_limiter_filter->Init();
    return FilterManager::GetInstance()->AddMessageClientFilter(fiber_limiter_filter);
  }
};

TEST_F(FiberLimiterClientTestFixture, Init) {
  MessageClientFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageClientFilter(kFiberLimiterName);
  ASSERT_NE(filter, nullptr);
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  ASSERT_EQ(points.size(), 2);
}

// Test to ensure that the number of fibers does not exceed the limit, to guarantee that flow control does not occur.
TEST_F(FiberLimiterClientTestFixture, Ok) {
  MessageClientFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageClientFilter(kFiberLimiterName);
  FiberLimiterClientFilter* fiber_filter = static_cast<FiberLimiterClientFilter*>(filter.get());

  uint32_t fiber_count = 10;
  FiberLatch l(fiber_count);

  for (uint32_t i = 0; i < fiber_count; i++) {
    ClientContextPtr context = MakeClientContext();
    FilterStatus status = FilterStatus::CONTINUE;
    fiber_filter->operator()(status, FilterPoint::CLIENT_PRE_SEND_MSG, context);
    fiber_filter->operator()(status, FilterPoint::CLIENT_POST_RECV_MSG, context);
    ASSERT_EQ(context->GetStatus().OK(), true);
    ASSERT_EQ(status, FilterStatus::CONTINUE);

    StartFiberDetached([this, &l]() { l.CountDown(); });
  }
  l.Wait();
}

// Test to ensure that the number of fibers exceeds the limit, causing flow control to occur.
TEST_F(FiberLimiterClientTestFixture, Overload) {
  MessageClientFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageClientFilter(kFiberLimiterName);
  FiberLimiterClientFilter* fiber_filter = static_cast<FiberLimiterClientFilter*>(filter.get());

  bool is_exist_reject = false;
  uint32_t fiber_count = 100;
  FiberLatch l(fiber_count);
  for (uint32_t i = 0; i < fiber_count; i++) {
    ClientContextPtr context = MakeClientContext();
    FilterStatus status = FilterStatus::CONTINUE;
    fiber_filter->operator()(status, FilterPoint::CLIENT_PRE_SEND_MSG, context);
    if (!context->GetStatus().OK()) {
      is_exist_reject = true;
      ASSERT_EQ(status, FilterStatus::REJECT);
    }

    StartFiberDetached([&l]() {
      FiberYield();
      l.CountDown();
    });
  }
  l.Wait();
  ASSERT_TRUE(is_exist_reject);
}
}  // namespace testing
}  // namespace trpc::overload_control

TEST_WITH_FIBER_THREAD_MAIN("./trpc/overload_control/fiber_limiter/fibers_overload_ctrl.yaml")
#endif
