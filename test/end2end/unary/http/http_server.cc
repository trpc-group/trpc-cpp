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

#include "test/end2end/unary/http/http_server.h"

#include "test/end2end/common/util.h"
#include "trpc/common/trpc_app.h"
#include "trpc/util/http/function_handlers.h"
#include "trpc/util/http/routes.h"

namespace trpc::testing {

::trpc::Status TestGetJson(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                           ::trpc::http::HttpReply* rsp) {
  rsp->SetContent(ConstructJsonStr({{"msg", "test-json"}}));
  return ::trpc::kSuccStatus;
}

::trpc::Status TestGetString(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                             ::trpc::http::HttpReply* rsp) {
  rsp->SetContent("test-rsp");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestGetEmptyBody(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                ::trpc::http::HttpReply* rsp) {
  return ::trpc::kSuccStatus;
}

::trpc::Status TestGet2(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                        ::trpc::http::HttpReply* rsp) {
  rsp->AddHeader("testkey", "testval");
  rsp->SetContent("test-rsp");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestGet2EmptyBody(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                 ::trpc::http::HttpReply* rsp) {
  rsp->AddHeader("testkey", "testval");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPostJson(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                            ::trpc::http::HttpReply* rsp) {
  ::rapidjson::Document req_json;
  req_json.Parse(req->GetContent().c_str());
  if (CheckJsonKV(req_json, "msg", "test-json-req")) {
    rsp->SetContent(ConstructJsonStr({{"msg", "test-json-rsp"}}));
  } else {
    rsp->SetContent(ConstructJsonStr({{"msg", "error"}}));
  }
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPostJsonRetString(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                     ::trpc::http::HttpReply* rsp) {
  rsp->SetContent("test-rsp");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPostJsonEmptyRsp(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                    ::trpc::http::HttpReply* rsp) {
  ::rapidjson::Document req_json;
  req_json.Parse(req->GetContent().c_str());
  if (CheckJsonKV(req_json, "msg", "test-json-req")) {
    rsp->SetContent("");
  } else {
    rsp->SetContent(ConstructJsonStr({{"msg", "error"}}));
  }
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPostJsonEmptyReq(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                    ::trpc::http::HttpReply* rsp) {
  ::rapidjson::Document req_json;
  req_json.Parse(req->GetContent().c_str());
  if (req_json.IsNull()) {
    rsp->SetContent(ConstructJsonStr({{"msg", "test-json-rsp"}}));
  } else {
    rsp->SetContent(ConstructJsonStr({{"msg", "error"}}));
  }
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPostString(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                              ::trpc::http::HttpReply* rsp) {
  if (req->GetContent() == "test-string-req") {
    rsp->SetContent("test-string-rsp");
  } else {
    rsp->SetContent("error");
  }
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPostStringEmptyRsp(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                      ::trpc::http::HttpReply* rsp) {
  if (req->GetContent() == "test-string-req") {
    rsp->SetContent("");
  } else {
    rsp->SetContent("error");
  }
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPostStringEmptyReq(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                      ::trpc::http::HttpReply* rsp) {
  if (req->GetContent().empty()) {
    rsp->SetContent("test-string-rsp");
  } else {
    rsp->SetContent("error");
  }
  return ::trpc::kSuccStatus;
}

::trpc::Status TestHead(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                        ::trpc::http::HttpReply* rsp) {
  rsp->AddHeader("Content-Length", "100");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestHeadWithContent(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                   ::trpc::http::HttpReply* rsp) {
  rsp->AddHeader("Content-Length", "100");
  rsp->SetContent("contentxxx");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestOptions(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                           ::trpc::http::HttpReply* rsp) {
  rsp->AddHeader("Allow", "OPTIONS, GET, HEAD, POST");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestGetJsonHeader(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                 ::trpc::http::HttpReply* rsp) {
  if (req->GetHeader("reqkey1") == "reqval1" && req->GetHeader("reqkey2") == "reqval2") {
    rsp->SetContent(ConstructJsonStr({{"msg", "test-json"}}));
    rsp->SetHeader("rspkey1", "rspval1");
    rsp->SetHeader("rspkey2", "rspval2");
  } else {
    rsp->SetContent(ConstructJsonStr({{"msg", "failed"}}));
  }
  return ::trpc::kSuccStatus;
}

::trpc::Status TestGetNot200Ret(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                                ::trpc::http::HttpReply* rsp) {
  rsp->SetStatus(400);
  rsp->SetContent("rsp");
  return ::trpc::kSuccStatus;
}

::trpc::Status TestPutJson(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                           ::trpc::http::HttpReply* rsp) {
  ::rapidjson::Document req_json;
  req_json.Parse(req->GetContent().c_str());
  if (CheckJsonKV(req_json, "msg", "test-json-req")) {
    rsp->SetContent(ConstructJsonStr({{"msg", "test-json-rsp"}}));
  } else {
    rsp->SetContent(ConstructJsonStr({{"msg", "error"}}));
  }
  return ::trpc::kSuccStatus;
}
::trpc::Status TestPutString(::trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                             ::trpc::http::HttpReply* rsp) {
  if (req->GetContent() == "test-string-req") {
    rsp->SetContent("test-string-rsp");
  } else {
    rsp->SetContent("error");
  }
  return ::trpc::kSuccStatus;
}

void SetHttpRoutes(::trpc::http::HttpRoutes& r) {
  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-json"),
        std::make_shared<::trpc::http::FuncHandler>(TestGetJson, "json"));
  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-json-empty"),
        std::make_shared<::trpc::http::FuncHandler>(TestGetEmptyBody, "json"));

  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-string"),
        std::make_shared<::trpc::http::FuncHandler>(TestGetString, "text"));
  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-string-empty"),
        std::make_shared<::trpc::http::FuncHandler>(TestGetEmptyBody, "text"));

  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-rsp-struct"),
        std::make_shared<::trpc::http::FuncHandler>(TestGet2, "text"));
  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-rsp-struct-empty"),
        std::make_shared<::trpc::http::FuncHandler>(TestGet2EmptyBody, "text"));

  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-json"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostJson, "json"));
  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-json-ret-string"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostJsonRetString, "text"));
  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-json-empty-rsp"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostJsonEmptyRsp, "json"));
  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-json-empty-req"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostJsonEmptyReq, "json"));

  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-string"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostString, "text"));
  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-string-empty-rsp"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostStringEmptyRsp, "text"));
  r.Add(::trpc::http::OperationType::POST, trpc::http::Path("/test-post-string-empty-req"),
        std::make_shared<::trpc::http::FuncHandler>(TestPostStringEmptyReq, "text"));

  r.Add(::trpc::http::OperationType::HEAD, trpc::http::Path("/test-head"),
        std::make_shared<::trpc::http::FuncHandler>(TestHead, "text"));
  r.Add(::trpc::http::OperationType::HEAD, trpc::http::Path("/test-head-with-content"),
        std::make_shared<::trpc::http::FuncHandler>(TestHeadWithContent, "text"));

  r.Add(::trpc::http::OperationType::OPTIONS, trpc::http::Path("/test-options"),
        std::make_shared<::trpc::http::FuncHandler>(TestOptions, "text"));

  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-json-header"),
        std::make_shared<::trpc::http::FuncHandler>(TestGetJsonHeader, "json"));

  r.Add(::trpc::http::OperationType::GET, trpc::http::Path("/test-get-not-200-ret"),
        std::make_shared<::trpc::http::FuncHandler>(TestGetNot200Ret, "text"));

  r.Add(::trpc::http::OperationType::PUT, trpc::http::Path("/test-put-json"),
        std::make_shared<::trpc::http::FuncHandler>(TestPutJson, "json"));
  r.Add(::trpc::http::OperationType::PUT, trpc::http::Path("/test-put-string"),
        std::make_shared<::trpc::http::FuncHandler>(TestPutString, "text"));
}

int HttpServer::Initialize() {
  auto http_service = std::make_shared<trpc::HttpService>();
  http_service->SetRoutes(SetHttpRoutes);

  RegisterService("default_http_service", http_service);

  test_signal_->SignalClientToContinue();

  return 0;
}

void HttpServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
