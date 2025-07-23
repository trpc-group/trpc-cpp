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

#include "trpc/metrics/trpc_metrics.h"

#include "trpc/metrics/metrics_factory.h"
#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/metrics/prometheus/prometheus_metrics.h"
#endif

namespace trpc::metrics {

bool Init() {
  // Registers default metrics plugins which provided by the framework.
#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
  MetricsFactory::GetInstance()->Register(MakeRefCounted<PrometheusMetrics>());
#endif

  return true;
}

void Stop() {}

void Destroy() { MetricsFactory::GetInstance()->Clear(); }

}  // namespace trpc::metrics
