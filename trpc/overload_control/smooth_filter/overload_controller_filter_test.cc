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

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/overload_control/flow_control/flow_controller.h"
#include "trpc/overload_control/smooth_filter/overload_controller_filter.h"
#include "trpc/overload_control/smooth_filter/smooth_limit_overload_controller.h"
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
  auto filter = OverloadControlFilter();
  filter.Init();
  auto filter_name = filter.Name();
  ASSERT_EQ(filter_name, "server_flow_control");
  ASSERT_NE(SmoothLimitOverloadController::GetInstance()->GetLimiter("trpc.test.helloworld.Greeter"), nullptr);
  ASSERT_NE(SmoothLimitOverloadController::GetInstance()->GetLimiter("/trpc.test.helloworld.Greeter/SayHello"),
            nullptr);
  ASSERT_NE(SmoothLimitOverloadController::GetInstance()->GetLimiter("/trpc.test.helloworld.Greeter/SayHelloAgain"),
            nullptr);
  ASSERT_NE(SmoothLimitOverloadController::GetInstance()->GetLimiter("/trpc.test.helloworld.Greeter/SayHelloAgain"),
            nullptr);
}

// Scenarios where requests are not intercepted after flow control is executed during testing.
TEST_F(FlowControlServerFilterTestFixture, Ok) {
  auto flow_control_filter = OverloadControlFilter();

  ServerContextPtr context = MakeServerContext();

  context->SetFuncName("Say");

  FilterStatus status = FilterStatus::CONTINUE;
  flow_control_filter.operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  // dirty request
  context->SetStatus(Status(-1, ""));
  flow_control_filter.operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), false);

  context->SetStatus(Status(0, ""));
  // service_->SetName("trpc.test.helloworld.Greeter");
  context->SetFuncName("SayHello");
  // Flow control is executed
  flow_control_filter.operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  // Testing invalid tracking points.
  flow_control_filter.operator()(status, FilterPoint::SERVER_PRE_SEND_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
