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
#include "trpc/metrics/prometheus/prometheus_client_filter.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/util/time.h"

namespace trpc {

int PrometheusClientFilter::Init() {
  inner_metrics_info_.env_namespace = TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace;
  inner_metrics_info_.env = TrpcConfig::GetInstance()->GetGlobalConfig().env_name;

  metrics_ = MetricsFactory::GetInstance()->Get(trpc::prometheus::kPrometheusMetricsName);
  if (!metrics_) {
    TRPC_LOG_ERROR("PrometheusClientFilter init failed: plugin prometheus has not been registered");
    return -1;
  }
  return 0;
}

std::vector<FilterPoint> PrometheusClientFilter::GetFilterPoint() {
  std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  return points;
}

void PrometheusClientFilter::operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) {
  if (!metrics_) {
    return;
  }

  if (point == FilterPoint::CLIENT_POST_RPC_INVOKE) {
    ModuleMetricsInfo info;
    info.source = kMetricsCallerSource;
    SetStatInfo(context, info.infos);
    info.cost_time = (trpc::time::GetMicroSeconds() - context->GetSendTimestampUs()) / 1000;

    metrics_->ModuleReport(info);
  }

  status = FilterStatus::CONTINUE;
}

void PrometheusClientFilter::SetStatInfo(const ClientContextPtr& ctx, std::map<std::string, std::string>& infos) {
  // set caller info
  trpc::prometheus::detail::SetCallerServiceInfo(ctx->GetCallerName(), infos);

  // set callee info
  trpc::prometheus::detail::SetCalleeServiceInfo(ctx->GetCalleeName(), infos);
  infos[trpc::prometheus::kCalleeFuncName] = ctx->GetFuncName();
  infos[trpc::prometheus::kCalleeIp] = ctx->GetIp();
  infos[trpc::prometheus::kCalleeContainerName] = ctx->GetTargetMetadata(naming::kNodeContainerName);
  if (!ctx->GetTargetMetadata(naming::kNodeSetName).empty()) {
    infos[trpc::prometheus::kCalleeSetId] = ctx->GetTargetMetadata(naming::kNodeSetName);
  }

  infos[trpc::prometheus::kNamespace] = inner_metrics_info_.env_namespace;
  infos[trpc::prometheus::kEnv] = inner_metrics_info_.env;
  infos[trpc::prometheus::kFrameRetCode] = std::to_string(ctx->GetStatus().GetFrameworkRetCode());
  infos[trpc::prometheus::kInterfaceRetCode] = std::to_string(ctx->GetStatus().GetFuncRetCode());
}

}  // namespace trpc
#endif
