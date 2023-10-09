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

void PrometheusHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                      rapidjson::Document::AllocatorType& alloc) {
  static std::unique_ptr<::prometheus::Serializer> serializer = std::make_unique<::prometheus::TextSerializer>();

  std::string prometheus_str = serializer->Serialize(trpc::prometheus::Collect());
  result.AddMember(rapidjson::StringRef("trpc-html"), rapidjson::Value(prometheus_str, alloc).Move(), alloc);
}

}  // namespace trpc::admin
#endif
