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

PrometheusHandler::PrometheusHandler() { description_ = "[GET /metrics] get prometheus metrics"; }

void PrometheusHandler::Init() {
  PrometheusConfig prometheus_conf;
  bool ret = TrpcConfig::GetInstance()->GetPluginConfig<PrometheusConfig>(
      "metrics", trpc::prometheus::kPrometheusMetricsName, prometheus_conf);
  if (!ret) {
    TRPC_LOG_WARN(
        "Failed to obtain Prometheus plugin configuration from the framework configuration file. Default configuration "
        "will be used.");
  }
  auth_cfg_ = prometheus_conf.auth_cfg;
}

bool PrometheusHandler::CheckTokenAuth(std::string bearer_token) {
  auto splited = Split(bearer_token, ' ');
  if (splited.size() != 2) {
    TRPC_FMT_ERROR("error token: {}", bearer_token);
    return false;
  }
  auto method = splited[0];
  if (method != "Bearer") {
    TRPC_FMT_ERROR("error auth method: {}", method);
    return false;
  }
  std::string token = std::string(splited[1]);
  if (!Jwt::isValid(token, auth_cfg_)) {
    TRPC_FMT_ERROR("error token: {}", token);
    return false;
  }
  return true;
}

bool PrometheusHandler::CheckBasicAuth(std::string token) {
  auto splited = Split(token, ' ');
  if (splited.size() != 2) {
    TRPC_FMT_ERROR("error token: {}", token);
    return false;
  }
  if (splited[0] != "Basic") {
    TRPC_FMT_ERROR("error token: {}", token);
    return false;
  }

  std::string username_pwd = http::Base64Decode(std::begin(splited[1]), std::end(splited[1]));
  auto sp = Split(username_pwd, ':');
  if (sp.size() != 2) {
    TRPC_FMT_ERROR("error token: {}", token);
    return false;
  }

  auto username = sp[0], pwd = sp[1];
  if (username != auth_cfg_["username"] || pwd != auth_cfg_["password"]) {
    TRPC_FMT_ERROR("error username or password: username: {}, password: {}", username, pwd);
    return false;
  }
  return true;
}

void PrometheusHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                      rapidjson::Document::AllocatorType& alloc) {
  static std::unique_ptr<::prometheus::Serializer> serializer = std::make_unique<::prometheus::TextSerializer>();

  std::string token = req->GetHeader("authorization");

  if (auth_cfg_.count("username") && auth_cfg_.count("password")) {
    TRPC_FMT_INFO("push mode");
    // push mode
    // use the basic auth if already config the username and password.
    if (!CheckBasicAuth(token)) {
      result.AddMember("message", "wrong request without right username or password", alloc);
      return;
    }
  } else {
    // pull mode
    // use the json web token auth.
    TRPC_FMT_INFO("pull mode");
    if (!CheckTokenAuth(token)) {
      result.AddMember("message", "wrong request without right token", alloc);
      return;
    }
  }

  std::string prometheus_str = serializer->Serialize(trpc::prometheus::Collect());
  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(prometheus_str, alloc).Move(), alloc);
}

}  // namespace trpc::admin
#endif
