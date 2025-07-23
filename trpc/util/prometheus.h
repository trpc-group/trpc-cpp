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
#include <vector>

#include "prometheus/counter.h"
#include "prometheus/gauge.h"
#include "prometheus/histogram.h"
#include "prometheus/registry.h"
#include "prometheus/summary.h"

namespace trpc::prometheus {

/// @brief Gets the globally default used registry.
/// @return Returns The globally default used registry.
std::shared_ptr<::prometheus::Registry> GetRegistry();

/// @brief Gets monitoring data collected by Prometheus.
std::vector<::prometheus::MetricFamily> Collect();

/// @brief Gets a counter type monitoring family.
::prometheus::Family<::prometheus::Counter>* GetCounterFamily(const char* name, const char* help,
                                                              const std::map<std::string, std::string>& labels = {});
/// @brief Gets a gauge type monitoring family.
::prometheus::Family<::prometheus::Gauge>* GetGaugeFamily(const char* name, const char* help,
                                                          const std::map<std::string, std::string>& labels = {});

/// @brief Gets a histogram type monitoring family.
::prometheus::Family<::prometheus::Histogram>* GetHistogramFamily(
    const char* name, const char* help, const std::map<std::string, std::string>& labels = {});

/// @brief Gets a summary type monitoring family.
::prometheus::Family<::prometheus::Summary>* GetSummaryFamily(const char* name, const char* help,
                                                              const std::map<std::string, std::string>& labels = {});

}  // namespace trpc::prometheus
#endif
