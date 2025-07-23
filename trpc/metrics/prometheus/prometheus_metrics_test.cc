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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/metrics/prometheus/prometheus_metrics.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"

namespace trpc::testing {

class PrometheusMetricsTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() { prometheus_metrics_ = MakeRefCounted<PrometheusMetrics>(); }
  static void TearDownTestCase() {}

 protected:
  static PrometheusMetricsPtr prometheus_metrics_;
};

PrometheusMetricsPtr PrometheusMetricsTest::prometheus_metrics_;

TEST_F(PrometheusMetricsTest, Init) {
  ASSERT_EQ(trpc::prometheus::kPrometheusMetricsName, prometheus_metrics_->Name());

  TrpcConfig::GetInstance()->Init("./trpc/metrics/prometheus/testing/prometheus_metrics.yaml");
  ASSERT_EQ(0, prometheus_metrics_->Init());
}

trpc::ModuleMetricsInfo GetTestModuleInfo(int call) {
  trpc::ModuleMetricsInfo info;
  info.source = call;
  info.cost_time = 1000;
  info.infos["module_key"] = "module_value";
  return info;
}

TEST_F(PrometheusMetricsTest, ModuleReport) {
  // 1. testing caller report
  auto caller_info = GetTestModuleInfo(0);
  ASSERT_EQ(0, prometheus_metrics_->ModuleReport(caller_info));
  ASSERT_EQ(0, prometheus_metrics_->ModuleReport(std::move(caller_info)));

  // 2. testing callee report
  auto callee_info = GetTestModuleInfo(1);
  ASSERT_EQ(0, prometheus_metrics_->ModuleReport(callee_info));
  ASSERT_EQ(0, prometheus_metrics_->ModuleReport(std::move(callee_info)));
}

trpc::SingleAttrMetricsInfo GetTestSingleInfo(trpc::MetricsPolicy type, std::string value) {
  trpc::SingleAttrMetricsInfo info;
  info.name = "single_key";
  info.dimension = value;
  info.value = 10;
  info.policy = type;
  return info;
}

TEST_F(PrometheusMetricsTest, SingleAttrReport) {
  // 1. testing report SET metrics data
  auto set_info = GetTestSingleInfo(trpc::SET, "single_set_value");
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(set_info));
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(std::move(set_info)));

  // 2. testing report SUM metrics data
  auto sum_info = GetTestSingleInfo(trpc::SUM, "single_sum_value");
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(sum_info));
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(std::move(sum_info)));

  // 3. testing report MID metrics data
  auto mid_info = GetTestSingleInfo(trpc::MID, "single_mid_value");
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(mid_info));
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(std::move(mid_info)));

  // 4. testing report QUANTILES metrics data
  auto quantiles_info = GetTestSingleInfo(trpc::QUANTILES, "single_quantiles_value");
  // report failed because quantiles filed is empty
  ASSERT_NE(0, prometheus_metrics_->SingleAttrReport(quantiles_info));
  // report failed because the value in quantiles do not have a size of 2
  quantiles_info.quantiles = {{0.5}};
  ASSERT_NE(0, prometheus_metrics_->SingleAttrReport(quantiles_info));
  // report success because the metrics data is valid
  quantiles_info.quantiles = {{0.5, 0.05}, {0.1, 0.05}};
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(quantiles_info));
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(std::move(quantiles_info)));

  // 5. testing report HISTOGRAM metrics data
  auto histogram_info = GetTestSingleInfo(trpc::HISTOGRAM, "single_histogram_value");
  // report failed because bucket filed is empty
  ASSERT_NE(0, prometheus_metrics_->SingleAttrReport(histogram_info));
  // report success because the metrics data is valid
  histogram_info.bucket = {0.1, 0.5, 1};
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(histogram_info));
  ASSERT_EQ(0, prometheus_metrics_->SingleAttrReport(std::move(histogram_info)));

  // 6. testing report not support type
  auto max_info = GetTestSingleInfo(trpc::MAX, "single_max_value");
  ASSERT_NE(0, prometheus_metrics_->SingleAttrReport(max_info));
  ASSERT_NE(0, prometheus_metrics_->SingleAttrReport(std::move(max_info)));
}

trpc::MultiAttrMetricsInfo GetTestMultiInfo(trpc::MetricsPolicy type, std::string value) {
  trpc::MultiAttrMetricsInfo info;
  info.tags = {{"multi_key1", value + "1"}, {"multi_key2", value + "2"}};
  info.values = {{type, 10}};
  return info;
}

TEST_F(PrometheusMetricsTest, MultiAttrReport) {
  // 1. report failed because values filed is empty
  trpc::MultiAttrMetricsInfo info;
  ASSERT_NE(0, prometheus_metrics_->MultiAttrReport(info));

  // 2. testing report SET metrics data
  auto set_info = GetTestMultiInfo(trpc::SET, "multi_set_value");
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(set_info));
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(std::move(set_info)));

  // 3. testing report SUM metrics data
  auto sum_info = GetTestMultiInfo(trpc::SUM, "multi_sum_value");
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(sum_info));
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(std::move(sum_info)));

  // 4. testing report MID metrics data
  auto mid_info = GetTestMultiInfo(trpc::MID, "multi_mid_value");
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(mid_info));
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(std::move(mid_info)));

  // 5. testing report QUANTILES metrics data
  auto quantiles_info = GetTestMultiInfo(trpc::QUANTILES, "multi_quantiles_value");
  // report failed because quantiles filed is empty
  ASSERT_NE(0, prometheus_metrics_->MultiAttrReport(quantiles_info));
  // report failed because the value in quantiles do not have a size of 2
  quantiles_info.quantiles = {{0.5}};
  ASSERT_NE(0, prometheus_metrics_->MultiAttrReport(quantiles_info));
  // report success because the metrics data is valid
  quantiles_info.quantiles = {{0.5, 0.05}, {0.1, 0.05}};
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(quantiles_info));
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(std::move(quantiles_info)));

  // 6. testing report HISTOGRAM metrics data
  auto histogram_info = GetTestMultiInfo(trpc::HISTOGRAM, "multi_histogram_value");
  // report failed because bucket filed is empty
  ASSERT_NE(0, prometheus_metrics_->MultiAttrReport(histogram_info));
  // report success because the metrics data is valid
  histogram_info.bucket = {0.1, 0.5, 1};
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(histogram_info));
  ASSERT_EQ(0, prometheus_metrics_->MultiAttrReport(std::move(histogram_info)));

  // 6. testing report not support type
  auto max_info = GetTestMultiInfo(trpc::MAX, "multi_max_value");
  ASSERT_NE(0, prometheus_metrics_->MultiAttrReport(max_info));
  ASSERT_NE(0, prometheus_metrics_->MultiAttrReport(std::move(max_info)));
}

}  // namespace trpc::testing
#endif
