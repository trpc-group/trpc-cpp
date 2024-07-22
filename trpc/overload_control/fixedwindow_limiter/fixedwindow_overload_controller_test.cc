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

#include "fixedwindow_overload_controller.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/server/server_context.h"

namespace trpc::overload_control::testing {

class FixedTimeWindowOverloadControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fixedwindow_overload_controller_ = std::make_shared<FixedTimeWindowOverloadController>(3, std::chrono::seconds(2));
    fixedwindow_overload_controller_->Init();
  }

  void TearDown() override {
    fixedwindow_overload_controller_->Destroy();
    fixedwindow_overload_controller_ = nullptr;
  }

 protected:
  std::shared_ptr<FixedTimeWindowOverloadController> fixedwindow_overload_controller_;
};

TEST(FixedTimeWindowOverloadController, Construct) {
  FixedTimeWindowOverloadController fixedwindow_overload_controller(100, std::chrono::seconds(1));
  ASSERT_TRUE(true);
}

TEST_F(FixedTimeWindowOverloadControllerTest, CheckLimit) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  context->SetRequestMsg(std::move(req_msg));
  context->SetFuncName("trpc.test.flow_control.fixedwindow_overload_controller_");
  
   // 第一个时间窗口内的限流行为
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(fixedwindow_overload_controller_->BeforeSchedule(context), true);
  }
  ASSERT_EQ(fixedwindow_overload_controller_->BeforeSchedule(context), false);

  // 超过限额，应该触发限流
  ASSERT_EQ(fixedwindow_overload_controller_->BeforeSchedule(context), false);

   //  等待时间窗口重置
  std::this_thread::sleep_for(std::chrono::seconds(2));

 // 第二个时间窗口内的限流行为
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(fixedwindow_overload_controller_->BeforeSchedule(context), true);
  }
  ASSERT_EQ(fixedwindow_overload_controller_->BeforeSchedule(context), false);
}

}  // namespace trpc::overload_control::testing

#endif

