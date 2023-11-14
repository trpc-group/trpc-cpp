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

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"

namespace trpc::testing {

TEST(RuntimeManager, Run) {
  bool running_flag{false};

  trpc::FiberLatch l(1);
  trpc::StartFiberDetached([&l, &running_flag] {
    running_flag = true;
    l.CountDown();
  });

  l.Wait();

  ASSERT_TRUE(running_flag);
}

}  // namespace trpc::testing

int InitAndRunAllTests(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int ret = ::trpc::TrpcConfig::GetInstance()->Init("./trpc/common/testing/fiber_testing.yaml");
  assert(ret == 0);

  return ::trpc::RunInTrpcRuntime([]() { return ::RUN_ALL_TESTS(); });
}

int main(int argc, char** argv) { return InitAndRunAllTests(argc, argv); }
