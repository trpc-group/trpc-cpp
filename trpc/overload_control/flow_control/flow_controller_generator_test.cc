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

#include "trpc/overload_control/flow_control/flow_controller_generator.h"

#include "gtest/gtest.h"

#include "trpc/overload_control/flow_control/flow_controller_conf.h"
#include "trpc/overload_control/flow_control/flow_controller_factory.h"

namespace trpc::overload_control {
namespace testing {

TEST(CreateFlowController, All) {
  ASSERT_TRUE(CreateFlowController("default(1000)", true, 20) != nullptr);
  ASSERT_TRUE(CreateFlowController("seconds(1000)", true, 0) != nullptr);
  ASSERT_TRUE(CreateFlowController("smooth(1000)", false, -1) != nullptr);
  ASSERT_TRUE(CreateFlowController("") == nullptr);
  ASSERT_TRUE(CreateFlowController("default(1000") == nullptr);
  ASSERT_TRUE(CreateFlowController("default(1000)abc") == nullptr);
  ASSERT_TRUE(CreateFlowController("default1000)") == nullptr);
  ASSERT_TRUE(CreateFlowController("(1000)") == nullptr);
  ASSERT_TRUE(CreateFlowController("default()") == nullptr);
  ASSERT_TRUE(CreateFlowController("default(-1)") == nullptr);
  ASSERT_TRUE(CreateFlowController("unknown(1000)") == nullptr);
}

TEST(RegisterFlowControl, All) {
  std::string service_name = "trpc.test.helloworld.Greeter";
  // service limiter invalid
  {
    FlowControlLimiterConf flow_conf;
    flow_conf.service_name = service_name;
    flow_conf.service_limiter = "";
    RegisterFlowController(flow_conf);
    ASSERT_FALSE(trpc::FlowControllerFactory::GetInstance()->GetFlowController(service_name));

    flow_conf.service_limiter = "default(100";
    RegisterFlowController(flow_conf);
    ASSERT_FALSE(trpc::FlowControllerFactory::GetInstance()->GetFlowController(service_name));
  }

  // service limiter valid
  {
    FlowControlLimiterConf flow_conf;

    flow_conf.service_name = service_name;
    FuncLimiterConfig func_limiter_conf;

    func_limiter_conf.name = "SayHello";
    func_limiter_conf.limiter = "default(1000)";

    FuncLimiterConfig func_limiter_conf1;
    func_limiter_conf1.name = "Func1";
    func_limiter_conf1.limiter = "";

    FuncLimiterConfig func_limiter_conf2;
    func_limiter_conf2.name = "Func2";
    func_limiter_conf2.limiter = "default(10";

    flow_conf.service_limiter = "default(1000)";
    flow_conf.func_limiters.push_back(func_limiter_conf);
    flow_conf.func_limiters.push_back(func_limiter_conf1);
    flow_conf.func_limiters.push_back(func_limiter_conf2);
    RegisterFlowController(flow_conf);
    ASSERT_TRUE(trpc::FlowControllerFactory::GetInstance()->GetFlowController(service_name));
    std::string prefix = std::string("/") + service_name;
    ASSERT_TRUE(FlowControllerFactory::GetInstance()->GetFlowController(prefix + "/SayHello"));
    ASSERT_FALSE(FlowControllerFactory::GetInstance()->GetFlowController(prefix + "/Func1"));
    ASSERT_FALSE(FlowControllerFactory::GetInstance()->GetFlowController(prefix + "/Func2"));
  }
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
