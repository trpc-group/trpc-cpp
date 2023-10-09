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

#include "trpc/common/status.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(StatusTest, Default) {
  trpc::Status status1;
  EXPECT_TRUE(status1.OK());

  trpc::Status status2(0, "hello");
  EXPECT_TRUE(status2.OK());

  status2.SetFrameworkRetCode(1);
  EXPECT_FALSE(status2.OK());

  status2.SetFuncRetCode(1);
  EXPECT_FALSE(status2.OK());

  EXPECT_EQ(status2.GetFrameworkRetCode(), 1);
  EXPECT_EQ(status2.GetFuncRetCode(), 1);
  status2.SetErrorMessage("hello world");
  EXPECT_EQ(status2.ErrorMessage(), "hello world");

  EXPECT_TRUE(kDefaultStatus.OK());

  trpc::Status ok_status;
  EXPECT_EQ(ok_status.ToString(), "OK");
}

}  // trpc::testing
