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
#include "trpc/admin/prometheus_handler.h"

#include "jwt-cpp/jwt.h"
#include "prometheus/text_serializer.h"

#include "trpc/metrics/prometheus/prometheus_conf_parser.h"

namespace {
bool JwtValidate(const std::string& token, std::map<std::string, std::string>& auth_cfg) {
  try {
    auto decoded_token = jwt::decode(token);

    auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{auth_cfg["secret"]});

    // Verify if the config exists.
    if (auth_cfg.count("iss")) {
      verifier.with_issuer(auth_cfg["iss"]);
    }
    if (auth_cfg.count("sub")) {
      verifier.with_subject(auth_cfg["sub"]);
    }
    if (auth_cfg.count("aud")) {
      verifier.with_audience(auth_cfg["aud"]);
    }

    verifier.verify(decoded_token);
    return true;
  } catch (const std::exception& e) {
    TRPC_FMT_ERROR("Jwt validate error: {}", e.what());
    return false;
  }
}
}  // namespace

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
  if (!JwtValidate(token, auth_cfg_)) {
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
  return true;
}

void PrometheusHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                      rapidjson::Document::AllocatorType& alloc) {
  static std::unique_ptr<::prometheus::Serializer> serializer = std::make_unique<::prometheus::TextSerializer>();

  std::string prometheus_str = serializer->Serialize(trpc::prometheus::Collect());

  std::string token = req->GetHeader("authorization");

  if (!auth_cfg_.empty()) {
    // use the json web token auth.
    if (!CheckTokenAuth(token)) {
      result.AddMember("message", "wrong request without right token", alloc);
      return;
    }
  }

  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(prometheus_str, alloc).Move(), alloc);
}

}  // namespace trpc::admin
#endif
