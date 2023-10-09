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

#include "trpc/metrics/trpc_metrics.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/testing/metrics_testing.h"

namespace trpc::testing {

class TrpcMetricsTest : public ::testing::Test {
 public:
  static void SetUpTestCase() { MetricsFactory::GetInstance()->Register(MakeRefCounted<TestMetrics>()); }

  static void TearDownTestCase() { MetricsFactory::GetInstance()->Clear(); }
};

TEST_F(TrpcMetricsTest, Init) {
  TrpcConfig::GetInstance()->Init("trpc/metrics/testing/metrics.yaml");
  ASSERT_TRUE(metrics::Init());
  metrics::Stop();
  metrics::Destroy();
}

}  // namespace trpc::testing
