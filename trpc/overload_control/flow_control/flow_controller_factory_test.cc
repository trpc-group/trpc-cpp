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

#include "trpc/overload_control/flow_control/flow_controller_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/filter/server_filter_manager.h"
#include "trpc/overload_control/flow_control/seconds_limiter.h"
#include "trpc/overload_control/flow_control/smooth_limiter.h"

namespace trpc::overload_control {
namespace testing {

TEST(FlowControllerFactory, Register) {
  FlowControllerPtr say_hello_controller(new trpc::SmoothLimiter(1000));
  FlowControllerFactory::GetInstance()->Register("/trpc.test.helloworld.Greeter/SayHello", say_hello_controller);

  trpc::FlowControllerPtr route_controller(new trpc::SecondsLimiter(10000));
  trpc::FlowControllerFactory::GetInstance()->Register("/trpc.test.helloworld.Greeter/Route", route_controller);

  ASSERT_EQ(FlowControllerFactory::GetInstance()->Size(), 2);
  ASSERT_EQ(FlowControllerFactory::GetInstance()->GetFlowController("/trpc.test.helloworld.Greeter/SayHello"),
            say_hello_controller);

  ASSERT_EQ(FlowControllerFactory::GetInstance()->GetFlowController("/trpc.test.helloworld.Greeter/Route"),
            route_controller);

  ASSERT_EQ(FlowControllerFactory::GetInstance()->GetFlowController("/trpc.test.helloworld.Greeter/Empty"), nullptr);

  FlowControllerFactory::GetInstance()->Clear();
}

}  // namespace testing

}  // namespace trpc::overload_control
#endif
