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

#include "test/end2end/unary/http/https_server.h"

#include "test/end2end/common/test_common_service.h"
#include "test/end2end/unary/http/http_rpc.trpc.pb.h"
#include "trpc/common/trpc_app.h"
#include "trpc/util/http/function_handlers.h"
#include "trpc/util/http/routes.h"

namespace trpc::testing {

class HttpRpcServiceImpl : public trpc::testing::httpsvr::HttpRpcService {
  ::trpc::Status TestPb(::trpc::ServerContextPtr context, const ::trpc::testing::httpsvr::TestRequest* request,
                        ::trpc::testing::httpsvr::TestReply* response) {
    if (request->msg() != "test-pb-req") {
      response->set_msg("failed");
    } else {
      response->set_msg("test-pb-rsp");
    }
    return ::trpc::kSuccStatus;
  }
};

::trpc::Status TestPostString(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                              ::trpc::http::HttpReply* rsp) {
  if (req->GetContent() == "test-string-req") {
    rsp->SetContent("test-string-rsp");
  } else {
    rsp->SetContent("error");
  }
  return ::trpc::kSuccStatus;
}

void SetHttpRoutes(::trpc::http::HttpRoutes& r) {
  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-string"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostString, "text"));
}

int HttpsServer::Initialize() {
  // Register three types of services:
  // xxx_no_ca: Do not use a certificate.
  // xxx_ca1: Use certificate ca1.
  // xxx_ca2: Use certificate ca2.

  auto http_service_no_ca = std::make_shared<::trpc::HttpService>();
  http_service_no_ca->SetRoutes(SetHttpRoutes);
  bool auto_start = true;
  RegisterService("http_service_no_ca", http_service_no_ca, auto_start);
  auto http_service_ca1 = std::make_shared<::trpc::HttpService>();
  http_service_ca1->SetRoutes(SetHttpRoutes);
  RegisterService("http_service_ca1", http_service_ca1, auto_start);
  auto http_service_ca2 = std::make_shared<::trpc::HttpService>();
  http_service_ca2->SetRoutes(SetHttpRoutes);
  RegisterService("http_service_ca2", http_service_ca2, auto_start);

  RegisterService("rpc_service_no_ca", std::make_shared<HttpRpcServiceImpl>(), auto_start);
  RegisterService("rpc_service_ca1", std::make_shared<HttpRpcServiceImpl>(), auto_start);
  RegisterService("rpc_service_ca2", std::make_shared<HttpRpcServiceImpl>(), auto_start);

  RegisterService("test_common_service", std::make_shared<::trpc::testing::TestCommonService>(this));

  test_signal_->SignalClientToContinue();

  return 0;
}

extern "C" void __gcov_flush();

void HttpsServer::Destroy() { __gcov_flush(); }

}  // namespace trpc::testing
