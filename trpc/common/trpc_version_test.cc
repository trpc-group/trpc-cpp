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

#include "trpc/common/trpc_version.h"


#include <string.h>
#include <iostream>

#include "gtest/gtest.h"

namespace trpc::testing {

class TestTrpcCppVersion : public ::testing::Test {
 protected:
  const char* trpc_cpp_version_;
};

TEST_F(TestTrpcCppVersion, CheckTrpcCppVersion) {
  trpc_cpp_version_ = "1.1.0";

  std::cout << "TRPC_Cpp_Version:" << TRPC_Cpp_Version() << std::endl;

  EXPECT_EQ(0, strcmp(trpc_cpp_version_, TRPC_Cpp_Version()));
}

}  // namespace trpc::testing
