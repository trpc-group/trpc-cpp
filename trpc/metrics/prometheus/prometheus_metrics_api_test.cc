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
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/prometheus/prometheus_metrics.h"

namespace trpc::testing {

class PrometheusMetricsAPITest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    prometheus_metrics_ = MakeRefCounted<PrometheusMetrics>();
    TrpcConfig::GetInstance()->Init("./trpc/metrics/prometheus/testing/prometheus_metrics.yaml");
    ASSERT_EQ(0, prometheus_metrics_->Init());
  }
  static void TearDownTestCase() {}

 protected:
  static PrometheusMetricsPtr prometheus_metrics_;
};

PrometheusMetricsPtr PrometheusMetricsAPITest::prometheus_metrics_;

std::map<std::string, std::string> GetTestLabels(std::string value) {
  std::map<std::string, std::string> labels = {{"inter_key1", value + "1"}, {"inter_key2", value + "2"}};
  return labels;
}

TEST_F(PrometheusMetricsAPITest, Interfaces) {
  // 1. testing report when prometheus plugin is not registered
  std::map<std::string, std::string> labels = GetTestLabels("inter_value");
  ASSERT_NE(0, trpc::prometheus::ReportSetMetricsInfo(labels, 10));

  // register the prometheus plugin
  MetricsFactory::GetInstance()->Register(prometheus_metrics_);

  // 2. testing report SET metrics data
  labels = GetTestLabels("inter_set_value");
  ASSERT_EQ(0, trpc::prometheus::ReportSetMetricsInfo(labels, 10));

  // 3. testing report SUM metrics data
  labels = GetTestLabels("inter_sum_value");
  ASSERT_EQ(0, trpc::prometheus::ReportSumMetricsInfo(labels, 10));

  // 4. testing report MID metrics data
  labels = GetTestLabels("inter_mid_value");
  ASSERT_EQ(0, trpc::prometheus::ReportMidMetricsInfo(labels, 10));

  // 5. testing report QUANTILES metrics data
  labels = GetTestLabels("inter_quantiles_value");
  // report failed because quantiles filed is empty
  ASSERT_NE(0, trpc::prometheus::ReportQuantilesMetricsInfo(labels, {}, 10));
  // report failed because the value in quantiles do not have a size of 2
  ASSERT_NE(0, trpc::prometheus::ReportQuantilesMetricsInfo(labels, {{0.5}}, 10));
  // report success because the metrics data is valid
  ASSERT_EQ(0, trpc::prometheus::ReportQuantilesMetricsInfo(labels, {{0.5, 0.05}, {0.1, 0.05}}, 10));

  // 6. testing report HISTOGRAM metrics data
  labels = GetTestLabels("inter_histogram_value");
  // report failed because bucket filed is empty
  ASSERT_NE(0, trpc::prometheus::ReportHistogramMetricsInfo(labels, {}, 10));
  // report success because the metrics data is valid
  trpc::HistogramBucket bucket = {0.1, 0.5, 1};
  ASSERT_EQ(0, trpc::prometheus::ReportHistogramMetricsInfo(labels, bucket, 10));
  ASSERT_EQ(0, trpc::prometheus::ReportHistogramMetricsInfo(labels, std::move(bucket), 10));
}

}  // namespace trpc::testing
#endif
