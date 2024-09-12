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

#include "trpc/overload_control/flow_control/flow_controller_server_filter.h"

#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/trpc_plugin.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/server/server_context.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/overload_control/flow_control/flow_controller.h"
#include "trpc/overload_control/smooth_filter/overload_controller_filter.h"
#include "trpc/overload_control/smooth_filter/window_limit_overload_controller.h"

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
    trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/smooth_filter/filter_test.yaml");
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
  auto overload_control = WindowLimitOverloadController();
  overload_control.Init();
  ASSERT_NE(overload_control->GetLimiter("trpc.test.helloworld.Greeter"), nullptr);
  ASSERT_NE(overload_control->GetLimiter("/trpc.test.helloworld.Greeter/SayHello"), nullptr);
  ASSERT_NE(WindowLimitOverloadController::GetInstance()->GetLimiter("/trpc.test.helloworld.Greeter/SayHelloAgain"),
            nullptr);
  ASSERT_NE(overload_control->GetLimiter("/trpc.test.helloworld.Greeter/SayHelloAgain"), nullptr);
  overload_control.Desroy();
  ASSERT_EQ(overload_control->GetLimiter("trpc.test.helloworld.Greeter"), nullptr);
  ASSERT_EQ(overload_control->GetLimiter("/trpc.test.helloworld.Greeter/SayHello"), nullptr);
  ASSERT_NE(WindowLimitOverloadController::GetInstance()->GetLimiter("/trpc.test.helloworld.Greeter/SayHelloAgain"),
            nullptr);
  ASSERT_EQ(overload_control->GetLimiter("/trpc.test.helloworld.Greeter/SayHelloAgain"), nullptr);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
