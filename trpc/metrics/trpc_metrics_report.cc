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

#include "trpc/metrics/metrics_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc::metrics {

namespace {

MetricsPtr GetPlugin(const std::string& name) {
  if (name.empty()) {
    return nullptr;
  }
  auto p = MetricsFactory::GetInstance()->Get(name);
  if (!p) {
    TRPC_LOG_DEBUG("metrics plugin: " << name << " get failed.");
  }
  return p;
}

template <typename T>
int ModuleReportTemplate(T&& info) {
  MetricsPtr metrics = GetPlugin(info.plugin_name);
  if (!metrics) {
    return -1;
  }

  return metrics->ModuleReport(std::forward<T>(info).module_info);
}

template <typename T>
int SingleAttrReportTemplate(T&& info) {
  MetricsPtr metrics = GetPlugin(info.plugin_name);
  if (!metrics) {
    return -1;
  }

  return metrics->SingleAttrReport(std::forward<T>(info).single_attr_info);
}

template <typename T>
int MultiAttrReportTemplate(T&& info) {
  MetricsPtr metrics = GetPlugin(info.plugin_name);
  if (!metrics) {
    return -1;
  }

  return metrics->MultiAttrReport(std::forward<T>(info).multi_attr_info);
}

}  // namespace

int ModuleReport(const TrpcModuleMetricsInfo& info) { return ModuleReportTemplate(info); }

int ModuleReport(TrpcModuleMetricsInfo&& info) { return ModuleReportTemplate(std::move(info)); }

int SingleAttrReport(const TrpcSingleAttrMetricsInfo& info) { return SingleAttrReportTemplate(info); }

int SingleAttrReport(TrpcSingleAttrMetricsInfo&& info) { return SingleAttrReportTemplate(std::move(info)); }

int MultiAttrReport(const TrpcMultiAttrMetricsInfo& info) { return MultiAttrReportTemplate(info); }

int MultiAttrReport(TrpcMultiAttrMetricsInfo&& info) { return MultiAttrReportTemplate(std::move(info)); }

}  // namespace trpc::metrics
