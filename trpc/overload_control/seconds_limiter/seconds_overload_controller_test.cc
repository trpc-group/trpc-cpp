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

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_protocol.h"

namespace trpc::overload_control {
namespace testing {

class SecondsOverloadControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    second_controller_ = std::make_shared<trpc::overload_control::SecondsOverloadController>(
        "trpc.test.flow_control.second_controller", 3, true);
    second_controller_->Init();
  }

  void TearDown() override {
    second_controller_->Destroy();
    second_controller_ = nullptr;
  }

 protected:
  std::shared_ptr<SecondsOverloadController> second_controller_;
};

TEST(SecondsOverloadController, Contruct) {
  SecondsOverloadController second_controller("1", 100, true, 0);
  SecondsOverloadController second_controller1("2", 100, false, -1);
  SecondsOverloadController second_controller2("3", 100, true, 100);
  ASSERT_TRUE(true);
  second_controller1.Init();
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ASSERT_EQ(second_controller1.BeforeSchedule(context), false);
}

TEST_F(SecondsOverloadControllerTest, CheckLimit) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  context->SetRequestMsg(std::move(req_msg));
  context->SetFuncName("trpc.test.flow_control.seconds_controller");

  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), true);

  std::this_thread::sleep_for(std::chrono::seconds(1));

  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), false);
  ASSERT_EQ(second_controller_->BeforeSchedule(context), true);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
