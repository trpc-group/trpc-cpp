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

#include "trpc/metrics/trpc_metrics_report.h"

#include "gtest/gtest.h"

#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/testing/metrics_testing.h"

namespace trpc::testing {

class TrpcMetricsReportTest : public ::testing::Test {
 public:
  static void SetUpTestCase() { MetricsFactory::GetInstance()->Register(MakeRefCounted<TestMetrics>()); }

  static void TearDownTestCase() { MetricsFactory::GetInstance()->Clear(); }
};

TEST_F(TrpcMetricsReportTest, ModuleReport) {
  TrpcModuleMetricsInfo info;
  info.plugin_name = kTestMetricsName;
  ASSERT_EQ(0, metrics::ModuleReport(info));
  ASSERT_EQ(0, metrics::ModuleReport(std::move(info)));

  TrpcModuleMetricsInfo invalid_info;
  invalid_info.plugin_name = "not_exist";
  ASSERT_NE(0, metrics::ModuleReport(invalid_info));
  ASSERT_NE(0, metrics::ModuleReport(std::move(invalid_info)));
}

TEST_F(TrpcMetricsReportTest, SingleAttrReport) {
  TrpcSingleAttrMetricsInfo info;
  info.plugin_name = kTestMetricsName;
  ASSERT_EQ(0, metrics::SingleAttrReport(info));
  ASSERT_EQ(0, metrics::SingleAttrReport(std::move(info)));

  TrpcSingleAttrMetricsInfo invalid_info;
  invalid_info.plugin_name = "not_exist";
  ASSERT_NE(0, metrics::SingleAttrReport(invalid_info));
  ASSERT_NE(0, metrics::SingleAttrReport(std::move(invalid_info)));
}

TEST_F(TrpcMetricsReportTest, MultiAttrReport) {
  TrpcMultiAttrMetricsInfo info;
  info.plugin_name = kTestMetricsName;
  ASSERT_EQ(0, metrics::MultiAttrReport(info));
  ASSERT_EQ(0, metrics::MultiAttrReport(std::move(info)));

  TrpcMultiAttrMetricsInfo invalid_info;
  invalid_info.plugin_name = "not_exist";
  ASSERT_NE(0, metrics::MultiAttrReport(invalid_info));
  ASSERT_NE(0, metrics::MultiAttrReport(std::move(invalid_info)));
}

}  // namespace trpc::testing
