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

#include "trpc/server/rpc_service_impl.h"
#include "trpc/stream/http/async/stream_function_handler.h"
#include "trpc/util/http/function_handlers.h"
#include "trpc/util/http/routes.h"

namespace trpc::stream {

/// @brief Definition of an asynchronous streaming http service. This service should be used instead of HttpService when
///        building services that handle chunked data in the default thread model. Additionally, this service can also
///        handle unary requests.
class HttpAsyncStreamService : public RpcServiceImpl {
 public:
  /// @brief This logic is not applicable to http streaming services.
  void HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept override {
    TRPC_ASSERT(false && "Impossible invoke");
  }

  /// @brief Dispatches and processes stream request. It includes checking for method existence, timeouts, interceptor
  ///        execution, and other related logic.
  void DispatchStream(const ServerContextPtr& context) noexcept override;

  /// @brief Checks if the methods in Service contains the streaming rpc method
  /// @note For HttpAsyncStreamService, this interface always returns true. Typically, the methods registered on
  ///       streaming services are streaming methods. Even if there are unary methods, they are internally encapsulated
  ///       based on streaming RPC methods.
  bool AnyOfStreamingRpc() override { return true; }

  /// @brief Registers method for asynchronous bidirectional streaming
  void RegisterAsyncStreamMethod(http::OperationType type, const http::Path& path,
                                 const HttpAsyncStreamFuncHandler::BidiStreamFunction& func) {
    routes_.Add(type, path, std::make_shared<HttpAsyncStreamFuncHandler>(func));
  }

  /// @brief Registers method for unary calls
  /// @note Asynchronous streaming should be programmed using a fully asynchronous programming style. This interface
  ///       exists for compatibility with old synchronous interface usage.
  void RegisterUnaryFuncHandler(http::OperationType type, const http::Path& path,
                                std::shared_ptr<http::HandlerBase> handler) {
    routes_.Add(type, path, handler);
  }

 private:
  void SendResponse(const ServerContextPtr& context, http::HttpResponse&& rsp);

  void HandleError(const ServerContextPtr& context, Status&& err_status, const HttpServerAsyncStreamPtr& stream);

  void HandleUnaryMethod(const ServerContextPtr& context, const HttpServerAsyncStreamPtr& stream,
                         http::HandlerBase* handler);

  void HandleUnaryMethodImpl(const ServerContextPtr& context, const HttpServerAsyncStreamPtr& stream,
                             http::FuncHandler* func_handler, http::HttpRequestPtr& req);

  void HandleStreamMethod(const ServerContextPtr& context, const HttpServerAsyncStreamPtr& stream,
                          http::HandlerBase* handler);

 private:
  http::HttpRoutes routes_;
};

}  // namespace trpc::stream
