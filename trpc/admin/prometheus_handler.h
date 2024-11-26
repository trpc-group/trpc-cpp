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

#include "trpc/admin/admin_handler.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/log/trpc_log.h"
#include "trpc/metrics/prometheus/prometheus_metrics.h"
#include "trpc/util/http/base64.h"
#include "trpc/util/jwt.h"
#include "trpc/util/prometheus.h"
#include "trpc/util/string/string_helper.h"
#include "trpc/util/time.h"

namespace trpc::admin {

/// @brief Handles the request for getting result of Prometheus monitor.
class PrometheusHandler : public AdminHandlerBase {
 public:
  PrometheusHandler();

  void Init();

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  bool CheckTokenAuth(std::string token);

  bool CheckBasicAuth(std::string token);

  std::map<std::string, std::string> auth_cfg_;
};

}  // namespace trpc::admin
#endif
