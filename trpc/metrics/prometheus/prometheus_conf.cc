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

#include "trpc/metrics/prometheus/prometheus_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void PrometheusConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("histogram_module_cfg:");
  for (size_t i = 0; i < histogram_module_cfg.size(); i++) {
    TRPC_LOG_DEBUG(i << " " << histogram_module_cfg[i]);
  }

  TRPC_LOG_DEBUG("const_labels:");
  for (auto label : const_labels) {
    TRPC_LOG_DEBUG(label.first << ":" << label.second);
  }

  TRPC_LOG_DEBUG("auth_cfg:");
  for (auto auth : auth_cfg) {
    TRPC_LOG_DEBUG(auth.first << ":" << auth.second);
  }

  TRPC_LOG_DEBUG("push_mode enable:" << push_mode.enable);
  TRPC_LOG_DEBUG("push_mode gateway_host:" << push_mode.gateway_host);
  TRPC_LOG_DEBUG("push_mode gateway_port:" << push_mode.gateway_port);
  TRPC_LOG_DEBUG("push_mode job_name:" << push_mode.job_name);
  TRPC_LOG_DEBUG("push_mode interval_ms:" << push_mode.interval_ms);

  TRPC_LOG_DEBUG("--------------------------------");
}

}  // namespace trpc
