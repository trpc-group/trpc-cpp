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
#include "trpc/util/http/status.h"
#include "trpc/util/http/stream/stream_handler.h"

#define TRPC_HTTP_STREAM_HANDLER_METHOD(x)                                                                          \
  virtual trpc::Status x(const trpc::ServerContextPtr&, const trpc::http::RequestPtr&, trpc::http::Response* rsp) { \
    rsp->SetStatus(trpc::http::ResponseStatus::kNotImplemented);                                                    \
    return trpc::kSuccStatus;                                                                                       \
  }
#define TRPC_HTTP_STREAM_HANDLER_CASE(x, f) \
  case x:                                   \
    status = f(ctx, req, rsp);              \
    break

namespace trpc::http {

/// @brief An HTTP handler to serve HTTP streaming request, it's easy to use.
/// Default implementations are provided for common HTTP method: GET, HEAD, POST, PUT, DELETE, OPTIONS, PATCH, and
/// user could override the them.
class HttpStreamHandler : public StreamHandlerBase {
 public:
  TRPC_HTTP_STREAM_HANDLER_METHOD(Get)
  TRPC_HTTP_STREAM_HANDLER_METHOD(Head)
  TRPC_HTTP_STREAM_HANDLER_METHOD(Post)
  TRPC_HTTP_STREAM_HANDLER_METHOD(Put)
  TRPC_HTTP_STREAM_HANDLER_METHOD(Delete)
  TRPC_HTTP_STREAM_HANDLER_METHOD(Options)
  TRPC_HTTP_STREAM_HANDLER_METHOD(Patch)

  virtual Status Handle(const ServerContextPtr& ctx, const RequestPtr& req, Response* rsp) {
    Status status;
    try {
      switch (req->GetMethodType()) {
        TRPC_HTTP_STREAM_HANDLER_CASE(GET, Get);
        TRPC_HTTP_STREAM_HANDLER_CASE(HEAD, Head);
        TRPC_HTTP_STREAM_HANDLER_CASE(POST, Post);
        TRPC_HTTP_STREAM_HANDLER_CASE(PUT, Put);
        TRPC_HTTP_STREAM_HANDLER_CASE(DELETE, Delete);
        TRPC_HTTP_STREAM_HANDLER_CASE(OPTIONS, Options);
        TRPC_HTTP_STREAM_HANDLER_CASE(PATCH, Patch);
        default:
          rsp->SetStatus(Response::StatusCode::kNotImplemented);
      }
    } catch (const BaseException& http_ex) {
      rsp->SetStatus(http_ex.Status(), http_ex.str());
    } catch (...) {  // do not return other exception messages to users for safety reasons
      rsp->SetStatus(Response::StatusCode::kInternalServerError);
    }
    rsp->Done();
    return status;
  }

  /// @brief Dispatches HTTP streaming request.
  /// Helper function to discard unused path parameter.
  /// Subclasses should override "virtual void Handle(ServerContextPtr, RequestPtr, Response*)" instead
  /// @private For internal use purpose only.
  Status HandleStream(const std::string&, ServerContextPtr ctx, RequestPtr req, Response* rsp) final {
    return Handle(ctx, req, rsp);
  }

  using StreamHandlerBase::Handle;
};

template <typename T>
class StreamHandlerFuncWrapper : public StreamHandlerBase {
 public:
  using TElem = typename std::pointer_traits<T>::element_type;
  using HandleFunc = Status (TElem::*)(const ServerContextPtr&, const RequestPtr&, Response*);

  StreamHandlerFuncWrapper(T handler_ptr, HandleFunc func) : handler_ptr_(std::move(handler_ptr)), func_(func) {}

  Status HandleStream(const std::string&, ServerContextPtr ctx, RequestPtr req, Response* rsp) override {
    return (*handler_ptr_.*func_)(ctx, req, rsp);
  }

 private:
  T handler_ptr_;
  HandleFunc func_;
};

}  // namespace trpc::http

#undef TRPC_HTTP_STREAM_HANDLER_METHOD
#undef TRPC_HTTP_STREAM_HANDLER_CASE
