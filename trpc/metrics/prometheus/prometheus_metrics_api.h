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
#pragma once

#include "trpc/metrics/metrics.h"
#include "trpc/metrics/prometheus/prometheus_common.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/util/prometheus.h"

/// @brief Prometheus metrics interfaces for user programing
namespace trpc::prometheus {

/// @brief Reports metrics data with SUM type
/// @param labels prometheus metrics labels
/// @param value the value to increment
/// @return report result
int ReportSumMetricsInfo(const std::map<std::string, std::string>& labels, double value);

/// @brief Reports metrics data with SET type
/// @param labels prometheus metrics labels
/// @param value the value to set
/// @return report result
int ReportSetMetricsInfo(const std::map<std::string, std::string>& labels, double value);

/// @brief Reports metrics data with MID type
/// @param labels prometheus metrics labels
/// @param value the value to observe
/// @return report result
int ReportMidMetricsInfo(const std::map<std::string, std::string>& labels, double value);

/// @brief Reports metrics data with QUANTILES type
/// @param labels prometheus metrics labels
/// @param quantiles the quantiles used to gather summary statistics
/// @param value the value to observe
/// @return report result
int ReportQuantilesMetricsInfo(const std::map<std::string, std::string>& labels, const SummaryQuantiles& quantiles,
                               double value);

/// @brief Reports metrics data with HISTOGRAM type
/// @param labels prometheus metrics labels
/// @param bucket the bucket used to gather histogram statistics
/// @param value the value to observe
/// @return report result
int ReportHistogramMetricsInfo(const std::map<std::string, std::string>& labels, const HistogramBucket& bucket,
                               double value);
int ReportHistogramMetricsInfo(const std::map<std::string, std::string>& labels, HistogramBucket&& bucket,
                               double value);

}  // namespace trpc::prometheus

#endif
