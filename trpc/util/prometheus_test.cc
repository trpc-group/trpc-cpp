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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include <memory>

#include "gtest/gtest.h"

#include "trpc/util/prometheus.h"

namespace trpc::testing {

TEST(PrometheusHandlerTest, GetFamily) {
  ASSERT_NE(nullptr, trpc::prometheus::GetCounterFamily("counter", "help", {}));
  ASSERT_NE(nullptr, trpc::prometheus::GetGaugeFamily("gauge", "help", {}));
  ASSERT_NE(nullptr, trpc::prometheus::GetHistogramFamily("histogram", "help", {}));
  ASSERT_NE(nullptr, trpc::prometheus::GetSummaryFamily("summary", "help", {}));
}

TEST(PrometheusHandlerTest, Collect) {
  std::vector<::prometheus::MetricFamily> metrics = trpc::prometheus::Collect();
  ASSERT_NE(0, metrics.size());
}

TEST(PrometheusHandlerTest, GetRegistry) {
  std::shared_ptr<::prometheus::Registry> registry = trpc::prometheus::GetRegistry();
  ASSERT_NE(nullptr, registry);
}

}  // namespace trpc::testing
#endif
