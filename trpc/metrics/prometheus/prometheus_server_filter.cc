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
#include "trpc/metrics/prometheus/prometheus_server_filter.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/util/time.h"

namespace trpc {

int PrometheusServerFilter::Init() {
  inner_metrics_info_.p_app = TrpcConfig::GetInstance()->GetServerConfig().app;
  inner_metrics_info_.p_server = TrpcConfig::GetInstance()->GetServerConfig().server;
  inner_metrics_info_.p_container_name = TrpcConfig::GetInstance()->GetGlobalConfig().container_name;
  inner_metrics_info_.set_name = TrpcConfig::GetInstance()->GetGlobalConfig().full_set_name;
  inner_metrics_info_.enable_set =
      TrpcConfig::GetInstance()->GetGlobalConfig().enable_set && !inner_metrics_info_.set_name.empty();
  inner_metrics_info_.p_ip = TrpcConfig::GetInstance()->GetGlobalConfig().local_ip;
  inner_metrics_info_.env_namespace = TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace;
  inner_metrics_info_.env = TrpcConfig::GetInstance()->GetGlobalConfig().env_name;

  metrics_ = MetricsFactory::GetInstance()->Get(trpc::prometheus::kPrometheusMetricsName);
  if (!metrics_) {
    TRPC_LOG_ERROR("PrometheusServerFilter init failed: plugin prometheus has not been registered");
    return -1;
  }
  return 0;
}

std::vector<FilterPoint> PrometheusServerFilter::GetFilterPoint() {
  std::vector<FilterPoint> points = {FilterPoint::SERVER_POST_RECV_MSG, FilterPoint::SERVER_PRE_SEND_MSG};
  return points;
}

void PrometheusServerFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
  if (!metrics_) {
    return;
  }

  if (point == FilterPoint::SERVER_PRE_SEND_MSG) {
    ModuleMetricsInfo info;
    info.source = kMetricsCalleeSource;
    SetStatInfo(context, info.infos);
    info.cost_time = trpc::time::GetMilliSeconds() - context->GetRecvTimestamp();

    metrics_->ModuleReport(info);
  }

  status = FilterStatus::CONTINUE;
}

void PrometheusServerFilter::SetStatInfo(const ServerContextPtr& ctx, std::map<std::string, std::string>& infos) {
  // set caller info
  trpc::prometheus::detail::SetCallerServiceInfo(ctx->GetCallerName(), infos);
  infos[trpc::prometheus::kCallerIp] = ctx->GetIp();

  // set callee info
  infos[trpc::prometheus::kCalleeAppKey] = inner_metrics_info_.p_app;
  infos[trpc::prometheus::kCalleeServerKey] = inner_metrics_info_.p_server;
  infos[trpc::prometheus::kCalleeServiceKey] = trpc::prometheus::detail::GetServiceName(ctx->GetCalleeName(), '.');
  infos[trpc::prometheus::kCalleeContainerName] = inner_metrics_info_.p_container_name;
  if (inner_metrics_info_.enable_set) {
    infos[trpc::prometheus::kCalleeSetId] = inner_metrics_info_.set_name;
  }
  infos[trpc::prometheus::kCalleeIp] = inner_metrics_info_.p_ip;
  infos[trpc::prometheus::kCalleeFuncName] = ctx->GetFuncName();

  infos[trpc::prometheus::kNamespace] = inner_metrics_info_.env_namespace;
  infos[trpc::prometheus::kEnv] = inner_metrics_info_.env;
  infos[trpc::prometheus::kFrameRetCode] = std::to_string(ctx->GetStatus().GetFrameworkRetCode());
  infos[trpc::prometheus::kInterfaceRetCode] = std::to_string(ctx->GetStatus().GetFuncRetCode());
}

}  // namespace trpc
#endif
