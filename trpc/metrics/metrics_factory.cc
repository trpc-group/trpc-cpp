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

#include "trpc/metrics/metrics_factory.h"

#include "trpc/util/log/logging.h"

namespace trpc {

bool MetricsFactory::Register(const MetricsPtr& metrics) {
  TRPC_ASSERT(metrics != nullptr);
  metrics_plugins_[metrics->Name()] = metrics;

  return true;
}

MetricsPtr MetricsFactory::Get(const std::string& name) const {
  auto iter = metrics_plugins_.find(name);
  if (iter != metrics_plugins_.end()) {
    return iter->second;
  }
  return nullptr;
}

const std::unordered_map<std::string, MetricsPtr>& MetricsFactory::GetAllPlugins() { return metrics_plugins_; }

void MetricsFactory::Clear() { metrics_plugins_.clear(); }

}  // namespace trpc
