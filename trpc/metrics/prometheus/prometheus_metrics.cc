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
#include "trpc/metrics/prometheus/prometheus_metrics.h"

#include "trpc/common/config/trpc_config.h"

namespace trpc {

int PrometheusMetrics::Init() noexcept {
  bool ret = TrpcConfig::GetInstance()->GetPluginConfig<PrometheusConfig>(
      "metrics", trpc::prometheus::kPrometheusMetricsName, prometheus_conf_);
  if (!ret) {
    TRPC_LOG_WARN(
        "Failed to obtain Prometheus plugin configuration from the framework configuration file. Default configuration "
        "will be used.");
  }
  prometheus_conf_.Display();

  // initialize prometheus metrics family
  rpc_client_counter_family_ =
      trpc::prometheus::GetCounterFamily(kRpcClientCounterName, kRpcClientCounterDesc, prometheus_conf_.const_labels);
  rpc_client_histogram_family_ = trpc::prometheus::GetHistogramFamily(kRpcClientHistogramName, kRpcClientHistogramDesc,
                                                                      prometheus_conf_.const_labels);
  rpc_server_counter_family_ =
      trpc::prometheus::GetCounterFamily(kRpcServerCounterName, kRpcServerCounterDesc, prometheus_conf_.const_labels);
  rpc_server_histogram_family_ = trpc::prometheus::GetHistogramFamily(kRpcServerHistogramName, kRpcServerHistogramDesc,
                                                                      prometheus_conf_.const_labels);

  prometheus_counter_family_ = trpc::prometheus::GetCounterFamily(kPrometheusCounterName, kPrometheusCounterDesc);
  prometheus_gauge_family_ = trpc::prometheus::GetGaugeFamily(kPrometheusGaugeName, kPrometheusGaugeDesc);
  prometheus_summary_family_ = trpc::prometheus::GetSummaryFamily(kPrometheusSummaryName, kPrometheusSummaryDesc);
  prometheus_histogram_family_ =
      trpc::prometheus::GetHistogramFamily(kPrometheusHistogramName, kPrometheusHistogramDesc);

  return 0;
}

int PrometheusMetrics::ModuleReport(const ModuleMetricsInfo& info) {
  // report the number of calls and the time it takes to execute.
  if (info.source == kMetricsCallerSource) {  // caller report
    auto& module_counter = rpc_client_counter_family_->Add(info.infos);
    module_counter.Increment();

    auto& module_histogram = rpc_client_histogram_family_->Add(info.infos, prometheus_conf_.histogram_module_cfg);
    module_histogram.Observe(info.cost_time);
  } else {  // callee report
    auto& module_counter = rpc_server_counter_family_->Add(info.infos);
    module_counter.Increment();

    auto& module_histogram = rpc_server_histogram_family_->Add(info.infos, prometheus_conf_.histogram_module_cfg);
    module_histogram.Observe(info.cost_time);
  }
  return 0;
}

int PrometheusMetrics::SetDataReport(const std::map<std::string, std::string>& labels, double value) {
  auto& gauge = prometheus_gauge_family_->Add(labels);
  gauge.Set(value);
  return 0;
}

int PrometheusMetrics::SumDataReport(const std::map<std::string, std::string>& labels, double value) {
  auto& counter = prometheus_counter_family_->Add(labels);
  counter.Increment(value);
  return 0;
}

int PrometheusMetrics::MidDataReport(const std::map<std::string, std::string>& labels, double value) {
  auto pro_quantiles = ::prometheus::Summary::Quantiles{{0.5, 0.05}};
  auto& summary = prometheus_summary_family_->Add(labels, std::move(pro_quantiles));
  summary.Observe(value);
  return 0;
}

int PrometheusMetrics::QuantilesDataReport(const std::map<std::string, std::string>& labels,
                                           const SummaryQuantiles& quantiles, double value) {
  if (quantiles.size() == 0) {
    TRPC_LOG_ERROR("quantiles size must > 0");
    return -1;
  }
  ::prometheus::Summary::Quantiles pro_quantiles;
  for (auto val : quantiles) {
    if (val.size() != 2) {
      TRPC_LOG_ERROR("each value in quantiles must have a size of 2");
      return -1;
    }
    pro_quantiles.emplace_back(::prometheus::detail::CKMSQuantiles::Quantile(val[0], val[1]));
  }
  auto& summary = prometheus_summary_family_->Add(labels, std::move(pro_quantiles));
  summary.Observe(value);
  return 0;
}
namespace {

template <typename T>
int HistogramDataReportTemplate(::prometheus::Family<::prometheus::Histogram>* family,
                                const std::map<std::string, std::string>& labels, T&& bucket, double value) {
  if (bucket.size() == 0) {
    TRPC_LOG_ERROR("bucket size must > 0");
    return -1;
  }
  auto& histogram = family->Add(labels, std::forward<T>(bucket));
  histogram.Observe(value);
  return 0;
}

}  // namespace

int PrometheusMetrics::HistogramDataReport(const std::map<std::string, std::string>& labels, HistogramBucket&& bucket,
                                           double value) {
  return HistogramDataReportTemplate(prometheus_histogram_family_, labels, std::move(bucket), value);
}

int PrometheusMetrics::HistogramDataReport(const std::map<std::string, std::string>& labels,
                                           const HistogramBucket& bucket, double value) {
  return HistogramDataReportTemplate(prometheus_histogram_family_, labels, bucket, value);
}

int PrometheusMetrics::SingleAttrReport(const SingleAttrMetricsInfo& info) { return SingleAttrReportTemplate(info); }

int PrometheusMetrics::SingleAttrReport(SingleAttrMetricsInfo&& info) {
  return SingleAttrReportTemplate(std::move(info));
}

int PrometheusMetrics::MultiAttrReport(const MultiAttrMetricsInfo& info) { return MultiAttrReportTemplate(info); }

int PrometheusMetrics::MultiAttrReport(MultiAttrMetricsInfo&& info) { return MultiAttrReportTemplate(std::move(info)); }

}  // namespace trpc
#endif
