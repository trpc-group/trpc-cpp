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

#include "trpc/overload_control/seconds_limiter/seconds_overload_controller.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/overload_control/flow_control/flow_controller_conf.h"

namespace trpc::overload_control {
namespace testing {

class SecondsOverloadControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    second_controller_ = std::make_shared<trpc::overload_control::SecondsOverloadController>();
    std::vector<FlowControlLimiterConf> flow_confs;
    FlowControlLimiterConf flow_conf;

    flow_conf.service_name = "trpc.test.helloworld.Greeter";

    FuncLimiterConfig func_limiter_conf1;
    func_limiter_conf1.name = "Func1";
    func_limiter_conf1.limiter = "seconds(1)";

    FuncLimiterConfig func_limiter_conf2;
    func_limiter_conf2.name = "Func2";
    func_limiter_conf2.limiter = "default(2)";

    flow_conf.service_limiter = "default(3)";
    flow_conf.func_limiters.push_back(func_limiter_conf1);
    flow_conf.func_limiters.push_back(func_limiter_conf2);

    flow_confs.push_back(flow_conf);
    second_controller_->Init(flow_confs);
  }

  void TearDown() override {
    second_controller_->Destroy();
    second_controller_ = nullptr;
  }

 protected:
  std::shared_ptr<SecondsOverloadController> second_controller_;
};

TEST_F(SecondsOverloadControllerTest, CheckLimit) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  context->SetRequestMsg(std::move(req_msg));
  context->SetCalleeName("trpc.test.helloworld.Greeter");

  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), true);

  std::this_thread::sleep_for(std::chrono::seconds(1));

  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), true);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  context->SetFuncName("/trpc.test.helloworld.Greeter/Func1");

  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), true);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  context->SetFuncName("/trpc.test.helloworld.Greeter/Func2");
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), true);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
