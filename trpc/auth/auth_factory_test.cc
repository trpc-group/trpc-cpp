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

#include "trpc/auth/auth_factory.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/auth/auth.h"
#include "trpc/auth/testing/auth_testing.h"

namespace trpc::testing {

class AuthFactoryTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(AuthFactoryTest, RegisterMockAuth) {
  auto test_auth = std::make_shared<TestAuth>();
  auto auth_factory = AuthFactory::GetInstance();

  ASSERT_EQ("TestAuth", test_auth->Name());
  ASSERT_EQ(PluginType::kAuth, test_auth->Type());

  // Empty factory.
  ASSERT_EQ(nullptr, auth_factory->Get(test_auth->Name()));

  // Register test auth.
  ASSERT_TRUE(auth_factory->Register(test_auth));
  ASSERT_FALSE(auth_factory->Register(test_auth));
  ASSERT_NE(nullptr, auth_factory->Get(test_auth->Name()));

  // Clear factory.
  auth_factory->Clear();
  ASSERT_EQ(nullptr, auth_factory->Get(test_auth->Name()));
}

}  // namespace trpc::testing
