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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include <map>
#include <string>
#include <vector>

#include "trpc/filter/filter.h"
#include "trpc/metrics/metrics.h"
#include "trpc/metrics/prometheus/prometheus_common.h"
#include "trpc/server/server_context.h"

namespace trpc {

class PrometheusServerFilter : public MessageServerFilter {
 public:
  int Init() override;

  std::string Name() override { return trpc::prometheus::kPrometheusMetricsName; }

  std::vector<FilterPoint> GetFilterPoint() override;

  void operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) override;

 private:
  void SetStatInfo(const ServerContextPtr& ctx, std::map<std::string, std::string>& infos);

 private:
  struct InnerMetricsInfo {
    std::string p_app;
    std::string p_server;
    std::string p_container_name;
    bool enable_set;
    std::string set_name;
    std::string p_ip;
    std::string env_namespace;
    std::string env;
  };

  // prometheus metrics instance
  MetricsPtr metrics_;

  InnerMetricsInfo inner_metrics_info_;
};

}  // namespace trpc
#endif
