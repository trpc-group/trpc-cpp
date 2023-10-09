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

#include <memory>
#include <string>
#include <utility>

#include "trpc/common/trpc_app.h"
#include "trpc/server/http_service.h"
#include "trpc/util/http/http_handler.h"
#include "trpc/util/http/routes.h"
#include "trpc/util/log/logging.h"

namespace http::demo {

// Another easy way to handle HTTP requests.
class FooHandler : public ::trpc::http::HttpHandler {
 public:
  ::trpc::Status Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                     ::trpc::http::Response* rsp) override {
    TRPC_LOG_INFO("request: " << req->SerializeToString());
    rsp->SetContent(greetings_);
    return ::trpc::kSuccStatus;
  }
  ::trpc::Status Head(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                      ::trpc::http::Response* rsp) override {
    TRPC_LOG_INFO("request: " << req->SerializeToString());
    rsp->SetHeader("Content-Length", std::to_string(greetings_.size()));
    return ::trpc::kSuccStatus;
  }
  ::trpc::Status Post(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                      ::trpc::http::Response* rsp) override {
    TRPC_LOG_INFO("request: " << req->SerializeToString());
    rsp->SetContent(req->GetContent());
    return ::trpc::kSuccStatus;
  }

 private:
  std::string greetings_{"hello world!"};
};

void SetHttpRoutes(::trpc::http::HttpRoutes& r) {
  auto foo_handler = std::make_shared<FooHandler>();
  r.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/foo"), foo_handler);
  r.Add(::trpc::http::MethodType::HEAD, ::trpc::http::Path("/foo"), foo_handler);
  r.Add(::trpc::http::MethodType::POST, ::trpc::http::Path("/foo"), foo_handler);
}

class HttpdServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    auto http_service = std::make_shared<::trpc::HttpService>();
    http_service->SetRoutes(SetHttpRoutes);

    RegisterService("https_service", http_service);

    return 0;
  }

  void Destroy() override {}
};

}  // namespace http::demo

int main(int argc, char** argv) {
  http::demo::HttpdServer httpd_server;

  httpd_server.Main(argc, argv);
  httpd_server.Wait();

  return 0;
}
