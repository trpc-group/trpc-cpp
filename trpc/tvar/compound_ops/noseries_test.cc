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

#include <chrono>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/tvar/compound_ops/latency_recorder.h"
#include "trpc/tvar/compound_ops/window.h"

namespace {

using trpc::tvar::LatencyRecorder;

}

namespace trpc::testing {

class TestTvarCompundOpsNoSeries : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/noseries.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

TEST_F(TestTvarCompundOpsNoSeries, LatencyRecorder) {
  LatencyRecorder recorder("l");

  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/l", true)->isObject());
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/l/qps")->asInt(), 0);

  recorder.Update(3);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/l/count")->asInt(), 1);
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/l/max_latency")->asInt(), 3);
}

}  // namespace trpc::testing
