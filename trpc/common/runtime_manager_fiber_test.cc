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
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/util/latch.h"

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

TEST(RuntimeManager, RunInTrpcRuntimeInTrpcRuntime) {
  // Test calling RunInTrpcRuntime again within RunInTrpcRuntime
  int ret = ::trpc::RunInTrpcRuntime([]() { return 0; });
  assert(ret != 0);
}

// Testing scenarios where InitFrameworkRuntime and DestroyFrameworkRuntime appear in pairs
int TestInitAndDestroyFrameworkRuntime() {
  int ret = ::trpc::InitFrameworkRuntime();
  assert(ret == 0);

  ::trpc::Latch l(1);
  int count = 0;
  bool start_fiber = trpc::StartFiberDetached([&] {
    count++;
    l.count_down();
  });

  if (start_fiber) {
    l.wait();
    assert(count == 1);
  }

  ret = ::trpc::DestroyFrameworkRuntime();
  assert(ret == 0);

  return 0;
}

// Test calling twice before and after RunInTrpc
int TestRunInTrpcRuntimeDouble() {
  bool running_flag{false};
  int ret = ::trpc::RunInTrpcRuntime([&running_flag]() {
    running_flag = true;

    return 0;
  });

  assert(ret == 0);
  assert(running_flag);

  return 0;
}

}  // namespace trpc::testing

int InitAndRunAllTests(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int ret = ::trpc::TrpcConfig::GetInstance()->Init("./trpc/common/testing/fiber_testing.yaml");
  assert(ret == 0);

  ret = ::trpc::testing::TestInitAndDestroyFrameworkRuntime();
  assert(ret == 0);

  ret = ::trpc::testing::TestRunInTrpcRuntimeDouble();
  assert(ret == 0);

  return ::trpc::RunInTrpcRuntime([]() { return ::RUN_ALL_TESTS(); });
}

int main(int argc, char** argv) { return InitAndRunAllTests(argc, argv); }
