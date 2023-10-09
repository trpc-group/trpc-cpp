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

#include "trpc/auth/auth_center_follower_factory.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/auth/auth.h"
#include "trpc/auth/testing/auth_testing.h"

namespace trpc::testing {

class AuthCenterFollowerFactoryTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(AuthCenterFollowerFactoryTest, RegisterMockAuthCenterFollower) {
  auto test_auth_center_follower = std::make_shared<TestAuthCenterFollower>();
  auto auth_center_follower_factory = AuthCenterFollowerFactory::GetInstance();

  ASSERT_EQ("TestAuthCenterFollower", test_auth_center_follower->Name());
  ASSERT_EQ(PluginType::kAuthCenterFollower, test_auth_center_follower->Type());

  std::any any_args;
  ASSERT_EQ(0, test_auth_center_follower->Expect(any_args));

  auto options = test_auth_center_follower->GetAuthOptions(any_args);
  ASSERT_EQ(0, std::any_cast<int>(options));

  // Empty factory.
  ASSERT_EQ(nullptr, auth_center_follower_factory->Get(test_auth_center_follower->Name()));

  // Register test auth center follower.
  ASSERT_TRUE(auth_center_follower_factory->Register(test_auth_center_follower));
  ASSERT_FALSE(auth_center_follower_factory->Register(test_auth_center_follower));
  ASSERT_NE(nullptr, auth_center_follower_factory->Get(test_auth_center_follower->Name()));

  // Clear factory.
  auth_center_follower_factory->Clear();
  ASSERT_EQ(nullptr, auth_center_follower_factory->Get(test_auth_center_follower->Name()));
}

}  // namespace trpc::testing
