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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/common/report.h"

#include "fmt/format.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {

Report::Report() { TrpcConfig::GetInstance()->GetPluginNodes("metrics", metrics_plugin_names_); }

void Report::ReportOverloadInfo(const OverloadInfo& overload_infos) {
  for (auto& metrics_plugin_name : metrics_plugin_names_) {
    for (const auto& [k, v] : overload_infos.tags) {
      TrpcSingleAttrMetricsInfo info;
      info.plugin_name = metrics_plugin_name;
      if (metrics_plugin_name != kPrometheusMetricsPlugin) {
        info.single_attr_info.name = overload_infos.attr_name;
        info.single_attr_info.dimension = fmt::format("{}/{}", overload_infos.report_name, k);
      } else {
        info.single_attr_info.name = k;
        info.single_attr_info.dimension = overload_infos.report_name;
      }
      info.single_attr_info.policy = MetricsPolicy::SET;
      info.single_attr_info.value = v;
      metrics::SingleAttrReport(info);
    }
  }
}

void Report::ReportStrategySampleInfo(const StrategySampleInfo& sample_infos) {
  for (auto& metrics_plugin_name : metrics_plugin_names_) {
    TrpcSingleAttrMetricsInfo info;
    info.plugin_name = metrics_plugin_name;

    if (sample_infos.strategy_name.empty()) {
      info.single_attr_info.name = kOverloadctrlHighPercentileCost;
      info.single_attr_info.dimension = fmt::format("{}/total", sample_infos.report_name);
    } else {
      info.single_attr_info.name = fmt::format("{}/{}", kOverloadctrlHighPercentile, sample_infos.strategy_name);
      info.single_attr_info.dimension = fmt::format("{}/{}/cost", sample_infos.report_name, sample_infos.strategy_name);
    }
    info.single_attr_info.value = sample_infos.value;
    info.single_attr_info.policy = MetricsPolicy::SET;
    metrics::SingleAttrReport(info);
  }
}

}  // namespace trpc::overload_control

#endif
