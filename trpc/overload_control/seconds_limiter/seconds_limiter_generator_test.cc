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

#include "trpc/overload_control/seconds_limiter/seconds_limiter_generator.h"

#include "gtest/gtest.h"

#include "trpc/overload_control/seconds_limiter/seconds_limiter_conf.h"
#include "trpc/overload_control/seconds_limiter/seconds_overload_controller.h"
#include "trpc/overload_control/seconds_limiter/seconds_overload_controller_factory.h"

namespace trpc::overload_control {
namespace testing {

TEST(CreateSecondsOverloadController, All) {
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "default(100)", true, 20) != nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "seconds(100)", false, -1) != nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "") == nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "default(100") == nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "default(100)abc") == nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "default100)") == nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "(100)") == nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "default()") == nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "default(-1)") == nullptr);
  ASSERT_TRUE(CreateSecondsOverloadController("trpc.test.helloworld.Greeter", "unknown(100)") == nullptr);
}

TEST(RegisterSecondsOverloadController, All) {
  std::string service_name = "trpc.test.helloworld.Greeter";
  // service limiter invalid
  {
    SecondsLimiterConf flow_conf;
    flow_conf.service_name = service_name;
    flow_conf.service_limiter = "";
    RegisterSecondsOverloadController(flow_conf);
    ASSERT_FALSE(nullptr != SecondsOverloadControllerFactory::GetInstance()->Get(service_name));

    flow_conf.service_limiter = "default(100";
    RegisterSecondsOverloadController(flow_conf);
    ASSERT_FALSE(nullptr != SecondsOverloadControllerFactory::GetInstance()->Get(service_name));
  }

  // service limiter valid
  {
    SecondsLimiterConf flow_conf;

    flow_conf.service_name = service_name;
    SecondsFuncLimiterConfig func_limiter_conf;

    func_limiter_conf.name = "SayHello";
    func_limiter_conf.limiter = "default(1000)";

    SecondsFuncLimiterConfig func_limiter_conf1;
    func_limiter_conf1.name = "Func1";
    func_limiter_conf1.limiter = "";

    SecondsFuncLimiterConfig func_limiter_conf2;
    func_limiter_conf2.name = "Func2";
    func_limiter_conf2.limiter = "default(10";

    flow_conf.service_limiter = "default(1000)";
    flow_conf.func_limiters.push_back(func_limiter_conf);
    flow_conf.func_limiters.push_back(func_limiter_conf1);
    flow_conf.func_limiters.push_back(func_limiter_conf2);
    RegisterSecondsOverloadController(flow_conf);
    ASSERT_TRUE(nullptr != SecondsOverloadControllerFactory::GetInstance()->Get(service_name));
    std::string prefix = std::string("/") + service_name;
    ASSERT_TRUE(nullptr != SecondsOverloadControllerFactory::GetInstance()->Get(prefix + "/SayHello"));
    ASSERT_FALSE(nullptr != SecondsOverloadControllerFactory::GetInstance()->Get(prefix + "/Func1"));
    ASSERT_FALSE(nullptr != SecondsOverloadControllerFactory::GetInstance()->Get(prefix + "/Func2"));

    SecondsOverloadControllerFactory::GetInstance()->Stop();
    SecondsOverloadControllerFactory::GetInstance()->Destroy();
  }
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
