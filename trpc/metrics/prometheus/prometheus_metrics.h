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
#pragma once

#include <map>
#include <memory>
#include <string>

#include "prometheus/counter.h"
#include "prometheus/family.h"
#include "prometheus/gauge.h"
#include "prometheus/histogram.h"
#include "prometheus/summary.h"

#include "trpc/metrics/metrics.h"
#include "trpc/metrics/prometheus/prometheus_common.h"
#include "trpc/metrics/prometheus/prometheus_conf.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/prometheus.h"

namespace trpc {

class PrometheusMetrics : public Metrics {
 public:
  std::string Name() const override { return trpc::prometheus::kPrometheusMetricsName; }

  int Init() noexcept override;

  void Start() noexcept override;

  void Stop() noexcept override;

  int ModuleReport(const ModuleMetricsInfo& info) override;

  int SingleAttrReport(const SingleAttrMetricsInfo& info) override;
  int SingleAttrReport(SingleAttrMetricsInfo&& info) override;

  int MultiAttrReport(const MultiAttrMetricsInfo& info) override;
  int MultiAttrReport(MultiAttrMetricsInfo&& info) override;

  /// @brief Reports metrics data with SET type
  /// @note This interface is for internal use only and should not be used by users. May be modified in the future.
  int SetDataReport(const std::map<std::string, std::string>& labels, double value);

  /// @brief Reports metrics data with SUM type
  /// @note This interface is for internal use only and should not be used by users. May be modified in the future.
  int SumDataReport(const std::map<std::string, std::string>& labels, double value);

  /// @brief Reports metrics data with MID type
  /// @note This interface is for internal use only and should not be used by users. May be modified in the future.
  int MidDataReport(const std::map<std::string, std::string>& labels, double value);

  /// @brief Reports metrics data with QUANTILES type
  /// @note This interface is for internal use only and should not be used by users. May be modified in the future.
  int QuantilesDataReport(const std::map<std::string, std::string>& labels, const SummaryQuantiles& quantiles,
                          double value);

  /// @brief Reports metrics data with HISTOGRAM type
  /// @note This interface is for internal use only and should not be used by users. May be modified in the future.
  int HistogramDataReport(const std::map<std::string, std::string>& labels, HistogramBucket&& bucket, double value);
  int HistogramDataReport(const std::map<std::string, std::string>& labels, const HistogramBucket& bucket,
                          double value);

 private:
  template <typename T>
  int SingleAttrReportTemplate(T&& info) {
    std::map<std::string, std::string> labels = {{std::forward<T>(info).name, std::forward<T>(info).dimension}};
    switch (info.policy) {
      case SET:
        return SetDataReport(labels, info.value);
      case SUM:
        return SumDataReport(labels, info.value);
      case MID:
        return MidDataReport(labels, info.value);
      case QUANTILES:
        return QuantilesDataReport(labels, info.quantiles, info.value);
      case HISTOGRAM:
        return HistogramDataReport(labels, std::forward<T>(info).bucket, info.value);
      default:
        TRPC_LOG_ERROR("unknown policy type: " << static_cast<int>(info.policy));
    }
    return -1;
  }

  template <typename T>
  int MultiAttrReportTemplate(T&& info) {
    if (info.values.empty()) {
      TRPC_LOG_ERROR("values can not be empty");
      return -1;
    }

    MetricsPolicy policy = info.values[0].first;
    double value = info.values[0].second;
    switch (policy) {
      case SET:
        return SetDataReport(info.tags, value);
      case SUM:
        return SumDataReport(info.tags, value);
      case MID:
        return MidDataReport(info.tags, value);
      case QUANTILES:
        return QuantilesDataReport(info.tags, info.quantiles, value);
      case HISTOGRAM:
        return HistogramDataReport(info.tags, std::forward<T>(info).bucket, value);
      default:
        TRPC_LOG_ERROR("unknown policy type: " << static_cast<int>(policy));
    }
    return -1;
  }

 private:
  PrometheusConfig prometheus_conf_;
  uint64_t push_gateway_task_id_ = 0;

  // metrics family for number of client-side RPC calls
  ::prometheus::Family<::prometheus::Counter>* rpc_client_counter_family_;
  static constexpr char kRpcClientCounterName[] = "rpc_client_counter_metric";
  static constexpr char kRpcClientCounterDesc[] = "Total number of RPCs started on the client.";
  // metrics family for distribution of client-side execution time
  ::prometheus::Family<::prometheus::Histogram>* rpc_client_histogram_family_;
  static constexpr char kRpcClientHistogramName[] = "rpc_client_histogram_metric";
  static constexpr char kRpcClientHistogramDesc[] = "Histogram of RPC response latency observed by client.";

  // metrics family for number of times the server is called
  ::prometheus::Family<::prometheus::Counter>* rpc_server_counter_family_;
  static constexpr char kRpcServerCounterName[] = "rpc_server_counter_metric";
  static constexpr char kRpcServerCounterDesc[] = "Total number of RPCs received by server.";
  // metrics family for distribution of server-side execution time
  ::prometheus::Family<::prometheus::Histogram>* rpc_server_histogram_family_;
  static constexpr char kRpcServerHistogramName[] = "rpc_server_histogram_metric";
  static constexpr char kRpcServerHistogramDesc[] = "Histogram of RPC response latency observed by server.";

  // custom metrics data
  ::prometheus::Family<::prometheus::Counter>* prometheus_counter_family_;
  static constexpr char kPrometheusCounterName[] = "prometheus_counter_metric";
  static constexpr char kPrometheusCounterDesc[] = "trpc-cpp prometheus counter metric.";
  ::prometheus::Family<::prometheus::Gauge>* prometheus_gauge_family_;
  static constexpr char kPrometheusGaugeName[] = "prometheus_gauge_metric";
  static constexpr char kPrometheusGaugeDesc[] = "trpc-cpp prometheus gauge metric.";
  ::prometheus::Family<::prometheus::Summary>* prometheus_summary_family_;
  static constexpr char kPrometheusSummaryName[] = "prometheus_summary_metric";
  static constexpr char kPrometheusSummaryDesc[] = "trpc-cpp prometheus summary metric.";
  ::prometheus::Family<::prometheus::Histogram>* prometheus_histogram_family_;
  static constexpr char kPrometheusHistogramName[] = "prometheus_histogram_metric";
  static constexpr char kPrometheusHistogramDesc[] = "trpc-cpp prometheus histogram metric.";
};

using PrometheusMetricsPtr = RefPtr<PrometheusMetrics>;

}  // namespace trpc
#endif
