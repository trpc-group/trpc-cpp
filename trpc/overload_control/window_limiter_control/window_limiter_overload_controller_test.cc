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

#include "trpc/overload_control/window_limiter_control/window_limiter_overload_controller.h"

#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/overload_control/flow_control/flow_controller.h"
#include "trpc/overload_control/flow_control/flow_controller_server_filter.h"
#include "trpc/overload_control/window_limiter_control/window_limiter_overload_controller_filter.h"
#include "trpc/server/server_context.h"

namespace trpc::overload_control {

namespace testing {

class MockFlowControl : public FlowController {
 public:
  MOCK_METHOD1(CheckLimit, bool(const ServerContextPtr&));
  MOCK_METHOD0(GetCurrCounter, int64_t());
  MOCK_CONST_METHOD0(GetMaxCounter, int64_t());
};

class FlowControlServerFilterTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/window_limiter_control/filter_test.yaml");
    trpc::TrpcPlugin::GetInstance()->RegisterPlugins();
  }

  static void TearDownTestCase() { trpc::TrpcPlugin::GetInstance()->UnregisterPlugins(); }

  static ServerContextPtr MakeServerContext() {
    auto context = MakeRefCounted<ServerContext>();
    context->SetRequestMsg(std::make_shared<trpc::testing::TestProtocol>());
    context->SetFuncName("trpc.test");
    return context;
  }
};

TEST_F(FlowControlServerFilterTestFixture, Init) {
  auto overload_control = WindowLimiterOverloadController();
  overload_control.Init();
  ServerContextPtr context = MakeServerContext();
  context->SetFuncName("Say");
  FilterStatus status = FilterStatus::CONTINUE;
  bool res = overload_control.BeforeSchedule(context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(res, true);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
