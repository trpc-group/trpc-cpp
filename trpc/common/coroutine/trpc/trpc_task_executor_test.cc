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

#include "trpc/common/coroutine/trpc/trpc_task_executor.h"

#include <mutex>

#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/runtime/runtime.h"

namespace trpc::coroutine {

std::mutex m_;

trpc::Status Add(int& n, int i) {
  std::scoped_lock _(m_);
  n += i;
  return trpc::kDefaultStatus;
}

trpc::Status MultiAdd(int& n) {
  for (int i = 0; i < 100; i++) {
    std::scoped_lock _(m_);
    n++;
  }
  return trpc::kDefaultStatus;
}

void TestTrpcTaskExecutor() {
  {
    TrpcTaskExecutor exec;
    int n = 0;
    for (int i = 0; i < 1; i++) {
      exec.AddTask([&n, i] { return Add(n, i); });
    }
    ASSERT_TRUE(exec.ExecTasks().OK());
    ASSERT_EQ(n, 0);
  }

  {
    TrpcTaskExecutor exec;
    int n = 0;
    for (int i = 0; i < 10; i++) {
      exec.AddTask([&n, i] { return Add(n, i); });
    }
    ASSERT_TRUE(exec.ExecTasks().OK());
    ASSERT_EQ(n, 45);
  }

  {
    trpc::coroutine::TrpcTaskExecutor exec;
    int n = 0;
    for (int i = 0; i < 100; i++) {
      exec.AddTask(MultiAdd, std::ref(n));
    }
    ASSERT_TRUE(exec.ExecTasks().OK());
    ASSERT_EQ(n, 10000);
  }
}

TEST(TrpcPeripheryTaskSchedulerTest, All) {
  trpc::runtime::SetRuntimeType(trpc::runtime::kFiberRuntime);

  RunAsFiber([] {
    TestTrpcTaskExecutor();

    std::cout << "running in fiber worker thread test ok." << std::endl;

    std::thread t(TestTrpcTaskExecutor);
    t.join();

    std::cout << "running in not fiber worker thread test ok." << std::endl;
  });
}

}  // namespace trpc::coroutine
