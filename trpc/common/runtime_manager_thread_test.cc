//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"

namespace trpc::testing {

TEST(RuntimeManager, RunThreadFunc) {
  bool running_flag{false};
  auto func = [&running_flag] {
    running_flag = true;
  };

  func();

  ASSERT_TRUE(running_flag);
}

}  // namespace trpc::testing

int InitAndRunAllTests(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int ret = ::trpc::TrpcConfig::GetInstance()->Init("./trpc/common/testing/merge_testing.yaml");
  assert(ret == 0);

  return ::trpc::RunInTrpcRuntime([]() { return ::RUN_ALL_TESTS(); });
}

int main(int argc, char** argv) { return InitAndRunAllTests(argc, argv); }
 