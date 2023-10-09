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

#include "trpc/metrics/metrics_factory.h"

#include "gtest/gtest.h"

#include "trpc/metrics/testing/metrics_testing.h"

namespace trpc::testing {

TEST(MetricsFactory, TestMetricsFactory) {
  MetricsPtr p = MakeRefCounted<TestMetrics>();
  MetricsFactory::GetInstance()->Register(p);
  ASSERT_EQ(p.Get(), MetricsFactory::GetInstance()->Get(kTestMetricsName).Get());

  ASSERT_EQ(1, MetricsFactory::GetInstance()->GetAllPlugins().size());

  MetricsFactory::GetInstance()->Clear();
  ASSERT_TRUE(MetricsFactory::GetInstance()->Get(kTestMetricsName) == nullptr);
}

}  // namespace trpc::testing
