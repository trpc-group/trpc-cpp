//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/smooth_filter/server_flow_controller_generator.h"
#include "trpc/overload_control/smooth_filter/server_flow_controller_conf.h"
#include "trpc/overload_control/smooth_filter/server_overload_controller_factory.h"
#include "trpc/overload_control/smooth_filter/server_smooth_limit.h"

#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(Server_CreateFlowController, All) {
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "default(1000)", true, 20) != nullptr);
  // ASSERT_TRUE(Server_CreateFlowController("seconds(1000)", true, 0) != nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "smooth(1000)", false, -1) != nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "") == nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "default(1000") == nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "default(1000)abc") == nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "default1000)") == nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "(1000)") == nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "default()") == nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "default(-1)") == nullptr);
  ASSERT_TRUE(Server_CreateFlowController("trpc.test.helloworld.Greeter", "unknown(1000)") == nullptr);
}

TEST(Server_RegisterFlowControl, All) {
  std::string service_name = "trpc.test.helloworld.Greeter";
  // service limiter invalid
  {
    Server_FlowControlLimiterConf flow_conf;
    flow_conf.service_name = service_name;
    flow_conf.service_limiter = "";
    Server_RegisterFlowController(flow_conf);
    ASSERT_FALSE(nullptr != ServerOverloadControllerFactory::GetInstance()->Get(service_name));

    flow_conf.service_limiter = "default(100";
    Server_RegisterFlowController(flow_conf);
    ASSERT_FALSE(nullptr != ServerOverloadControllerFactory::GetInstance()->Get(service_name));
  }

  // service limiter valid
  {
    Server_FlowControlLimiterConf flow_conf;

    flow_conf.service_name = service_name;
    Server_FuncLimiterConfig func_limiter_conf;

    func_limiter_conf.name = "SayHello";
    func_limiter_conf.limiter = "default(1000)";

    Server_FuncLimiterConfig func_limiter_conf1;
    func_limiter_conf1.name = "Func1";
    func_limiter_conf1.limiter = "";

    Server_FuncLimiterConfig func_limiter_conf2;
    func_limiter_conf2.name = "Func2";
    func_limiter_conf2.limiter = "default(10";

    flow_conf.service_limiter = "default(1000)";
    flow_conf.func_limiters.push_back(func_limiter_conf);
    flow_conf.func_limiters.push_back(func_limiter_conf1);
    flow_conf.func_limiters.push_back(func_limiter_conf2);
    Server_RegisterFlowController(flow_conf);
    ASSERT_TRUE(nullptr != ServerOverloadControllerFactory::GetInstance()->Get(service_name));
    std::string prefix = std::string("/") + service_name;
    ASSERT_TRUE(nullptr != ServerOverloadControllerFactory::GetInstance()->Get(prefix + "/SayHello"));
    ASSERT_FALSE(nullptr != ServerOverloadControllerFactory::GetInstance()->Get(prefix + "/Func1"));
    ASSERT_FALSE(nullptr != ServerOverloadControllerFactory::GetInstance()->Get(prefix + "/Func2"));

    ServerOverloadControllerFactory::GetInstance()->Stop();
    ServerOverloadControllerFactory::GetInstance()->Destroy();
  }
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
