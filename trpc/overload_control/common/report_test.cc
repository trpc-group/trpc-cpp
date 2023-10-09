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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/common/report.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/testing/metrics_testing.h"
#include "trpc/metrics/trpc_metrics.h"

namespace trpc::overload_control {
namespace testing {

class ReportTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    MetricsFactory::GetInstance()->Register(MakeRefCounted<trpc::testing::TestMetrics>());
    ASSERT_EQ(TrpcConfig::GetInstance()->Init("./trpc/overload_control/common/report_test.yaml"), 0);
    ASSERT_EQ(metrics::Init(), true);

    mock_metrics_ = MetricsFactory::GetInstance()->Get("test_metrics");
  }

  static void TearDownTestCase() { metrics::Destroy(); }

  static MetricsPtr mock_metrics_;
};

MetricsPtr ReportTest::mock_metrics_;

TEST_F(ReportTest, OverloadInfo) {
  auto overload_infos =
      OverloadInfo{.attr_name = "test",
                   .report_name = "Greeter/SayHello",
                   .tags = {{"lower", 0.1}, {"upper", 0.2}, {"must", 0.1}, {"may", 0.1}, {"ok", 0.1}, {"no", 0.1}}};

  auto* mock_metrics = static_cast<trpc::testing::TestMetrics*>(mock_metrics_.get());

  ASSERT_TRUE(mock_metrics != nullptr);
  Report::GetInstance()->ReportOverloadInfo(overload_infos);
}

TEST_F(ReportTest, ReportStrategySampleInfo) {
  auto sample_infos = StrategySampleInfo();
  sample_infos.report_name = "Greeter/SayHello";
  sample_infos.strategy_name = "";
  sample_infos.value = 1;
  Report::GetInstance()->ReportStrategySampleInfo(sample_infos);
  sample_infos.strategy_name = "test";
  Report::GetInstance()->ReportStrategySampleInfo(sample_infos);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
