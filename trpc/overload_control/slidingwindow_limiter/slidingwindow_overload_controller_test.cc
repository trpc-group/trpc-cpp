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

#include "slidingwindow_overload_controller.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/server/server_context.h"

namespace trpc::overload_control::testing {

class FixedTimeWindowOverloadControllerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    slidingwindow_overload_controller_ = std::make_shared<SlidingWindowOverloadController>(3, std::chrono::seconds(2));
    slidingwindow_overload_controller_->Init();
  }

  void TearDown() override {
    slidingwindow_overload_controller_->Destroy();
    slidingwindow_overload_controller_ = nullptr;
  }

 protected:
  std::shared_ptr<SlidingWindowOverloadController> slidingwindow_overload_controller_;
};

TEST(SlidingWindowOverloadController, Construct) {
  SlidingWindowOverloadController slidingwindow_overload_controller(100, std::chrono::seconds(1));
  ASSERT_TRUE(true);
}

TEST_F(SlidingWindowOverloadControllerTest, CheckLimit) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  context->SetRequestMsg(std::move(req_msg));
  context->SetFuncName("trpc.test.flow_control.slidingwindow_overload_controller_");

   // Current limiting behavior in the first time window
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(slidingwindow_overload_controller_->BeforeSchedule(context), true);
  }
  ASSERT_EQ(slidingwindow_overload_controller_->BeforeSchedule(context), false);

  ASSERT_EQ(slidingwindow_overload_controller_->BeforeSchedule(context), false);

   //  Waiting time window reset
  std::this_thread::sleep_for(std::chrono::seconds(2));

 // Current limiting behavior in the second time window
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(slidingwindow_overload_controller_->BeforeSchedule(context), true);
  }
  ASSERT_EQ(slidingwindow_overload_controller_->BeforeSchedule(context), false);
}

}  // namespace trpc::overload_control::testing

#endif