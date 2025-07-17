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

#include "trpc/util/http/exception.h"
#include "trpc/util/http/handler.h"
#include "trpc/util/http/status.h"

#define TRPC_HTTP_HANDLER_METHOD(x)                                                                                 \
  virtual trpc::Status x(const trpc::ServerContextPtr&, const trpc::http::RequestPtr&, trpc::http::Response* rsp) { \
    rsp->SetStatus(trpc::http::ResponseStatus::kNotImplemented);                                                    \
    return trpc::kSuccStatus;                                                                                       \
  }
#define TRPC_HTTP_HANDLER_CASE(x, f) \
  case x:                            \
    status = f(ctx, req, rsp);       \
    break

namespace trpc::http {

/// @brief An HTTP handler to serve HTTP request, it's easy to use.
/// Default implementations are provided for common HTTP method: GET, HEAD, POST, PUT, DELETE, OPTIONS, PATCH, and
/// user could override the them.
class HttpHandler : public HandlerBase {
 public:
  TRPC_HTTP_HANDLER_METHOD(Get)
  TRPC_HTTP_HANDLER_METHOD(Head)
  TRPC_HTTP_HANDLER_METHOD(Post)
  TRPC_HTTP_HANDLER_METHOD(Put)
  TRPC_HTTP_HANDLER_METHOD(Delete)
  TRPC_HTTP_HANDLER_METHOD(Options)
  TRPC_HTTP_HANDLER_METHOD(Patch)

  /// @brief Dispatches HTTP request.
  virtual Status Handle(const ServerContextPtr& ctx, const HttpRequestPtr& req, HttpResponse* rsp) {
    Status status;
    try {
      switch (req->GetMethodType()) {
        TRPC_HTTP_HANDLER_CASE(GET, Get);
        TRPC_HTTP_HANDLER_CASE(HEAD, Head);
        TRPC_HTTP_HANDLER_CASE(POST, Post);
        TRPC_HTTP_HANDLER_CASE(PUT, Put);
        TRPC_HTTP_HANDLER_CASE(DELETE, Delete);
        TRPC_HTTP_HANDLER_CASE(OPTIONS, Options);
        TRPC_HTTP_HANDLER_CASE(PATCH, Patch);
        default:
          rsp->SetStatus(ResponseStatus::kNotImplemented);
      }
    } catch (const BaseException& http_ex) {
      rsp->SetStatus(http_ex.Status(), http_ex.str());
    } catch (...) {  // Do not return other exception messages to users for safety reasons.
      rsp->SetStatus(ResponseStatus::kInternalServerError);
    }
    rsp->Done();
    return status;
  }

  /// @brief Dispatches HTTP request.
  /// Helper function to discard unused path parameter.
  /// Subclasses should override "virtual void Handle(const ServerContextPtr&, const HttpRequestPtr&, HttpResponse*)"
  /// instead.
  /// @private For internal use purpose only.
  Status Handle(const std::string&, ServerContextPtr ctx, HttpRequestPtr req, HttpResponse* rsp) final {
    return Handle(ctx, req, rsp);
  }
};

}  // namespace trpc::http

#undef TRPC_HTTP_HANDLER_METHOD
#undef TRPC_HTTP_HANDLER_CASE
