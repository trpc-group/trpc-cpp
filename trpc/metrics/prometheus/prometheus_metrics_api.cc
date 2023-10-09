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
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/prometheus/prometheus_metrics.h"

namespace trpc::prometheus {

namespace {

PrometheusMetricsPtr GetPlugin() {
  MetricsPtr metrics = MetricsFactory::GetInstance()->Get(trpc::prometheus::kPrometheusMetricsName);
  if (!metrics) {
    TRPC_FMT_DEBUG("get metrics plugin {} failed.", trpc::prometheus::kPrometheusMetricsName);
    return nullptr;
  }
  PrometheusMetricsPtr pro_metrics = trpc::dynamic_pointer_cast<PrometheusMetrics>(metrics);
  if (!pro_metrics) {
    TRPC_FMT_DEBUG("the type of {} instance is invalid.", trpc::prometheus::kPrometheusMetricsName);
    return nullptr;
  }
  return pro_metrics;
}

template <typename T>
int ReportHistogramTemplate(const std::map<std::string, std::string>& labels, T&& bucket, double value) {
  trpc::PrometheusMetricsPtr metrics = GetPlugin();
  if (!metrics) {
    return -1;
  }
  return metrics->HistogramDataReport(labels, std::forward<T>(bucket), value);
}

}  // namespace

int ReportSumMetricsInfo(const std::map<std::string, std::string>& labels, double value) {
  trpc::PrometheusMetricsPtr metrics = GetPlugin();
  if (!metrics) {
    return -1;
  }
  return metrics->SumDataReport(labels, value);
}

int ReportSetMetricsInfo(const std::map<std::string, std::string>& labels, double value) {
  trpc::PrometheusMetricsPtr metrics = GetPlugin();
  if (!metrics) {
    return -1;
  }
  return metrics->SetDataReport(labels, value);
}

int ReportMidMetricsInfo(const std::map<std::string, std::string>& labels, double value) {
  trpc::PrometheusMetricsPtr metrics = GetPlugin();
  if (!metrics) {
    return -1;
  }
  return metrics->MidDataReport(labels, value);
}

int ReportQuantilesMetricsInfo(const std::map<std::string, std::string>& labels, const SummaryQuantiles& quantiles,
                               double value) {
  trpc::PrometheusMetricsPtr metrics = GetPlugin();
  if (!metrics) {
    return -1;
  }
  return metrics->QuantilesDataReport(labels, quantiles, value);
}

int ReportHistogramMetricsInfo(const std::map<std::string, std::string>& labels, const HistogramBucket& bucket,
                               double value) {
  return ReportHistogramTemplate(labels, bucket, value);
}

int ReportHistogramMetricsInfo(const std::map<std::string, std::string>& labels, HistogramBucket&& bucket,
                               double value) {
  return ReportHistogramTemplate(labels, std::move(bucket), value);
}

}  // namespace trpc::prometheus

#endif
