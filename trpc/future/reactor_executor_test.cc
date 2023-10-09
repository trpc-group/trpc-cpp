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

#include "trpc/future/reactor_executor.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/future/future.h"
#include "trpc/runtime/merge_runtime.h"

namespace trpc {

namespace testing {

// Test ReactorExecutor in merge thread model.
TEST(ReactorExecutorTest, MergeTest) {
  bool ret = TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml");
  EXPECT_EQ(ret, 0);
  merge::StartRuntime();

  int num = runtime::GetMergeThreadNum();
  for (int i = 0; i < num; ++i) {
    EXPECT_NE(ReactorExecutor::Index(i), nullptr);
    runtime::GetReactor(i)->SubmitTask([]() {
      auto exe = ReactorExecutor::ThreadLocal();
      EXPECT_NE(exe, nullptr);
      MakeReadyFuture<>().Then(exe, []() {
        return MakeReadyFuture<>();
      });
    });
  }

  merge::TerminateRuntime();
}

}  // namespace testing

}  // namespace trpc
