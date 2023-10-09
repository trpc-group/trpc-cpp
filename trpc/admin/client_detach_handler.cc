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

#include "trpc/admin/client_detach_handler.h"

#include <mutex>

#include "trpc/admin/base_funcs.h"
#include "trpc/client/trpc_client.h"
#include "trpc/client/trpc_service_proxy.h"
#include "trpc/util/http/body_params.h"
#include "trpc/util/log/logging.h"

namespace trpc::admin {

void ClientDetachHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                        rapidjson::Document::AllocatorType& alloc) {
  trpc::http::BodyParam body_param(req->GetContent());
  auto service_name = body_param.GetBodyParam("service_name");
  if (service_name.empty()) {
    result.AddMember("message", "bad request without service name", alloc);
    return;
  }

  // get client config and check service exist
  bool service_exist = false;
  const trpc::ClientConfig& client_config = trpc::TrpcConfig::GetInstance()->GetClientConfig();
  for (std::size_t i = 0; i < client_config.service_proxy_config.size(); i++) {
    if (client_config.service_proxy_config[i].name == service_name) {
      service_exist = true;
      break;
    }
  }

  if (service_exist == false) {
    result.AddMember("message", "service is not exist", alloc);
    return;
  }

  auto remote_ip = body_param.GetBodyParam("remote_ip");
  if (remote_ip.empty()) {
    result.AddMember("message", "bad request without remote_ip", alloc);
    return;
  }

  auto proxy = GetTrpcClient()->GetProxy<ServiceProxy>(service_name);
  proxy->Disconnect(remote_ip);

  result.AddMember("client_detach", "success.", alloc);
}

}  // namespace trpc::admin
