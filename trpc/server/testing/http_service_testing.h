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

#pragma once

#include <string>

#include "trpc/util/http/http_handler.h"
#include "trpc/util/http/routes.h"
#include "trpc/util/http/stream/http_stream_handler.h"

namespace trpc::testing {

void SetHttpRoutes(trpc::http::HttpRoutes& r);

bool TestHttpUnaryCall(const std::string& ip, int port);

class TestHttpFooHandler : public trpc::http::HttpHandler {
 public:
  trpc::Status Get(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                   trpc::http::Response* rsp) override;
  trpc::Status Head(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                    trpc::http::Response* rsp) override;
  trpc::Status Post(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                    trpc::http::Response* rsp) override;

 private:
  std::string greetings_{"hello world!"};
};

class TestHttpBarHandler : public trpc::http::HttpHandler {
 public:
  trpc::Status Handle(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                      trpc::http::Response* rsp) override;

 private:
  std::string greetings_{"hello world!"};
};

class TestHttpStreamHandler : public trpc::http::HttpStreamHandler {
 public:
  // Provides file downloading.
  trpc::Status Get(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                   trpc::http::Response* rsp) override;
  // Provides file uploading.
  trpc::Status Post(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                    trpc::http::Response* rsp) override;
  // Just for testing purposes only: reset the stream.
  trpc::Status Put(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                   trpc::http::Response* rsp) override;
};

}  // namespace trpc::testing
