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
#include "trpc/admin/prometheus_handler.h"

#include "prometheus/text_serializer.h"

namespace trpc::admin {

PrometheusHandler::PrometheusHandler() { 
  description_ = "[GET /metrics] get prometheus metrics";
  bool ret = TrpcConfig::GetInstance()->GetPluginConfig<PrometheusConfig>(
      "metrics", trpc::prometheus::kPrometheusMetricsName, prometheus_conf_);
  if (!ret) {
    TRPC_LOG_WARN(
        "Failed to obtain Prometheus plugin configuration from the framework configuration file. Default configuration "
        "will be used.");
  }
  Init();
}

void PrometheusHandler::Init() {
  auto& cfg = prometheus_conf_.auth_cfg;
  if (cfg.count("username") && cfg.count("password")) {
    auth_conf_.username = cfg["username"];
    auth_conf_.password = cfg["password"];
  } else {
    TRPC_LOG_INFO("can not found prometheus auth config");
  }
}

void PrometheusHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                      rapidjson::Document::AllocatorType& alloc) {
  static std::unique_ptr<::prometheus::Serializer> serializer = std::make_unique<::prometheus::TextSerializer>();

  if (auth_conf_.username.size() && auth_conf_.password.size()) {
    std::string token = req->GetHeader("Authorization");
    auto splited = Split(token, ' ');
    if (splited.size() != 2) {
      result.AddMember("message", "wrong request without authorization", alloc);
      TRPC_LOG_INFO("error token: " << token);
      return;
    }
    if (splited[0] != "Basic") {
      result.AddMember("message", "wrong request without right auth", alloc);
      TRPC_LOG_INFO("error token: " << token);
      return;
    }

    std::string username_pwd = http::Base64Decode(std::begin(splited[1]), std::end(splited[1]));
    auto sp = Split(username_pwd, ':');
    if (sp.size() != 2) {
      result.AddMember("message", "wrong request without authorization", alloc);
      TRPC_LOG_INFO("error token: " << token);
      return;
    }
    
    auto username = sp[0];
    if (username != auth_conf_.username) {
      result.AddMember("message", "wrong request without right username", alloc);
      TRPC_LOG_INFO("error username: " << username << ",right username: " << auth_conf_.username);
      return;
    }
    auto pwd = sp[1];
    if (pwd != auth_conf_.password) {
      result.AddMember("message", "wrong request without right password", alloc);
      TRPC_LOG_INFO("error password: " << pwd << ",right password: " << auth_conf_.password);
      return;
    }
  }

  std::string prometheus_str = serializer->Serialize(trpc::prometheus::Collect());
  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(prometheus_str, alloc).Move(), alloc);
}

}  // namespace trpc::admin
#endif
