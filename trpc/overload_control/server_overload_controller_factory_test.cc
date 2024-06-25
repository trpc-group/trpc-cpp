// Copyright (c) 2024, Tencent Inc.
// All rights reserved.

#include "trpc/overload_control/server_overload_controller_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/log/trpc_log.h"
#include "trpc/overload_control/testing/overload_control_testing.h"

namespace trpc::overload_control {

namespace testing {

TEST(ServerOverloadControllerFactory, All) {
  // Testing register interface
  {
    // 1. Register nullptr, failed.
    ASSERT_FALSE(ServerOverloadControllerFactory::GetInstance()->Register(nullptr));
    // 2. First time register, succ.
    ServerOverloadControllerPtr controller = std::make_shared<MockServerOverloadController>();
    MockServerOverloadController* mock_controller = static_cast<MockServerOverloadController*>(controller.get());
    EXPECT_CALL(*mock_controller, Init()).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mock_controller, Name()).WillRepeatedly(::testing::Return(std::string("mock_controller")));
    ASSERT_FALSE(ServerOverloadControllerFactory::GetInstance()->Register(controller));

    EXPECT_CALL(*mock_controller, Init()).WillOnce(::testing::Return(true));
    ASSERT_TRUE(ServerOverloadControllerFactory::GetInstance()->Register(controller));
    // 3. Duplicated register, failed.
    ASSERT_FALSE(ServerOverloadControllerFactory::GetInstance()->Register(controller));

    auto size = ServerOverloadControllerFactory::GetInstance()->Size();
    ASSERT_EQ(size, 1);
  }
  // Testing get interface
  {
    ServerOverloadControllerPtr controller = ServerOverloadControllerFactory::GetInstance()->Get("xxx");
    ASSERT_EQ(controller, nullptr);
    controller = ServerOverloadControllerFactory::GetInstance()->Get("mock_controller");
    ASSERT_NE(controller, nullptr);
  }

  // Testing series of cleaning interface
  {
    ServerOverloadControllerFactory::GetInstance()->Stop();
    ServerOverloadControllerFactory::GetInstance()->Destroy();
    auto size = ServerOverloadControllerFactory::GetInstance()->Size();
    ASSERT_EQ(size, 0);
  }
}

}  // namespace testing

}  // namespace trpc::overload_control
