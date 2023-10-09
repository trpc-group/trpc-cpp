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

#pragma once

#include "trpc/metrics/trpc_metrics_report.h"

namespace trpc {

/// @brief Metrics interfaces for user programing
/// @note These interfaces will be deprecated, please use the interfaces in trpc_metrics_report.h.
class TrpcMetrics {
 public:
  [[deprecated("use trpc::metrics::ModuleReport(const TrpcModuleMetricsInfo&) instead")]]
  static int ModuleReport(const TrpcModuleMetricsInfo* info);
  [[deprecated("use trpc::metrics::ModuleReport(const TrpcModuleMetricsInfo&) instead")]]
  static int ModuleReport(const TrpcModuleMetricsInfo& info);
  [[deprecated("use trpc::metrics::ModuleReport(TrpcModuleMetricsInfo&&) instead")]]
  static int ModuleReport(TrpcModuleMetricsInfo&& info);

  [[deprecated("use trpc::metrics::SingleAttrReport(const TrpcSingleAttrMetricsInfo&) instead")]]
  static int SingleAttrReport(const TrpcSingleAttrMetricsInfo* info);
  [[deprecated("use trpc::metrics::SingleAttrReport(const TrpcSingleAttrMetricsInfo&) instead")]]
  static int SingleAttrReport(const TrpcSingleAttrMetricsInfo& info);
  [[deprecated("use trpc::metrics::SingleAttrReport(TrpcSingleAttrMetricsInfo&&) instead")]]
  static int SingleAttrReport(TrpcSingleAttrMetricsInfo&& info);

  [[deprecated("use trpc::metrics::MultiAttrReport(const TrpcMultiAttrMetricsInfo&) instead")]]
  static int MultiAttrReport(const TrpcMultiAttrMetricsInfo* info);
  [[deprecated("use trpc::metrics::MultiAttrReport(const TrpcMultiAttrMetricsInfo&) instead")]]
  static int MultiAttrReport(const TrpcMultiAttrMetricsInfo& info);
  [[deprecated("use trpc::metrics::MultiAttrReport(TrpcMultiAttrMetricsInfo&&) instead")]]
  static int MultiAttrReport(TrpcMultiAttrMetricsInfo&& info);
};

}  // namespace trpc
