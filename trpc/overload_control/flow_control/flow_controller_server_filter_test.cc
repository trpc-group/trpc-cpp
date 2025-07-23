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
#include "trpc/overload_control/flow_control/flow_controller_factory.h"
#include "trpc/overload_control/flow_control/flow_controller_generator.h"
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
    trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/flow_control/flow_test.yaml");
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
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kFlowControlName);
  ASSERT_NE(filter, nullptr);
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  ASSERT_EQ(points.size(), 2);

  ASSERT_NE(FlowControllerFactory::GetInstance()->GetFlowController("trpc.test.helloworld.Greeter"), nullptr);
  ASSERT_NE(FlowControllerFactory::GetInstance()->GetFlowController("/trpc.test.helloworld.Greeter/SayHello"), nullptr);
  ASSERT_NE(FlowControllerFactory::GetInstance()->GetFlowController("/trpc.test.helloworld.Greeter/Route"), nullptr);
  ASSERT_NE(FlowControllerFactory::GetInstance()->GetFlowController("trpc.test.flow.Greeter"), nullptr);
  ASSERT_NE(FlowControllerFactory::GetInstance()->GetFlowController("/trpc.test.flow.Greeter/SayHello1"), nullptr);
  ASSERT_NE(FlowControllerFactory::GetInstance()->GetFlowController("/trpc.test.flow.Greeter/Route1"), nullptr);
}

// Scenarios where requests are not intercepted after flow control is executed during testing.
TEST_F(FlowControlServerFilterTestFixture, Ok) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kFlowControlName);
  FlowControlServerFilter* flow_control_filter = static_cast<FlowControlServerFilter*>(filter.get());

  ServerContextPtr context = MakeServerContext();

  context->SetFuncName("Say");

  FilterStatus status = FilterStatus::CONTINUE;
  flow_control_filter->operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  // dirty request
  context->SetStatus(Status(-1, ""));
  flow_control_filter->operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), false);

  context->SetStatus(Status(0, ""));
  // service_->SetName("trpc.test.helloworld.Greeter");
  context->SetFuncName("SayHello");
  // Flow control is executed
  flow_control_filter->operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  // Testing invalid tracking points.
  flow_control_filter->operator()(status, FilterPoint::SERVER_PRE_SEND_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

// Scenarios where requests are intercepted after flow control is executed during testing.
TEST_F(FlowControlServerFilterTestFixture, Overload) {
  std::shared_ptr<MockFlowControl> mock_control = std::make_shared<MockFlowControl>();
  FlowControllerFactory::GetInstance()->Register("test_service", mock_control);
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kFlowControlName);
  FlowControlServerFilter* flow_control_filter = static_cast<FlowControlServerFilter*>(filter.get());

  EXPECT_CALL(*mock_control, CheckLimit(::testing::_)).WillOnce(::testing::Return(true));
  ServerContextPtr context = MakeServerContext();
  context->SetCalleeName("test_service");
  context->SetFuncName("Say");
  FilterStatus status = FilterStatus::CONTINUE;
  flow_control_filter->operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(status, FilterStatus::REJECT);

  status = FilterStatus::CONTINUE;
  context->SetStatus(Status(0, ""));

  std::shared_ptr<MockFlowControl> mock_control1 = std::make_shared<MockFlowControl>();
  FlowControllerFactory::GetInstance()->Register("test_service/Say", mock_control1);
  context->SetCalleeName("test_service1");
  context->SetFuncName("test_service/Say");
  EXPECT_CALL(*mock_control1, CheckLimit(::testing::_)).WillOnce(::testing::Return(true));
  flow_control_filter->operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(status, FilterStatus::REJECT);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
