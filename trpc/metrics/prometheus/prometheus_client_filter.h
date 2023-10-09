
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

#include <map>
#include <string>
#include <vector>

#include "trpc/client/client_context.h"
#include "trpc/filter/filter.h"
#include "trpc/metrics/metrics.h"
#include "trpc/metrics/prometheus/prometheus_common.h"

namespace trpc {

class PrometheusClientFilter : public MessageClientFilter {
 public:
  int Init() override;

  std::string Name() override { return trpc::prometheus::kPrometheusMetricsName; }

  std::vector<FilterPoint> GetFilterPoint() override;

  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override;

 private:
  void SetStatInfo(const ClientContextPtr& ctx, std::map<std::string, std::string>& infos);

 private:
  struct InnerMetricsInfo {
    std::string env_namespace;
    std::string env;
  };

  // prometheus metrics instance
  MetricsPtr metrics_;

  InnerMetricsInfo inner_metrics_info_;
};

}  // namespace trpc
#endif
