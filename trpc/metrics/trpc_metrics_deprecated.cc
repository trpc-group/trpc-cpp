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

#include "trpc/metrics/trpc_metrics_deprecated.h"

namespace trpc {

int TrpcMetrics::ModuleReport(const TrpcModuleMetricsInfo* info) { return metrics::ModuleReport(*info); }

int TrpcMetrics::ModuleReport(const TrpcModuleMetricsInfo& info) { return metrics::ModuleReport(info); }

int TrpcMetrics::ModuleReport(TrpcModuleMetricsInfo&& info) { return metrics::ModuleReport(std::move(info)); }

int TrpcMetrics::SingleAttrReport(const TrpcSingleAttrMetricsInfo* info) { return metrics::SingleAttrReport(*info); }

int TrpcMetrics::SingleAttrReport(const TrpcSingleAttrMetricsInfo& info) { return metrics::SingleAttrReport(info); }

int TrpcMetrics::SingleAttrReport(TrpcSingleAttrMetricsInfo&& info) {
  return metrics::SingleAttrReport(std::move(info));
}

int TrpcMetrics::MultiAttrReport(const TrpcMultiAttrMetricsInfo* info) { return metrics::MultiAttrReport(*info); }

int TrpcMetrics::MultiAttrReport(const TrpcMultiAttrMetricsInfo& info) { return metrics::MultiAttrReport(info); }

int TrpcMetrics::MultiAttrReport(TrpcMultiAttrMetricsInfo&& info) { return metrics::MultiAttrReport(std::move(info)); }

}  // namespace trpc
