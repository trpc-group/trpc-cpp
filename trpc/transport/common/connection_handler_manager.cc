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

#include "trpc/transport/common/connection_handler_manager.h"

#include "trpc/stream/grpc/grpc_client_stream_connection_handler.h"
#include "trpc/stream/grpc/grpc_server_stream_connection_handler.h"
#include "trpc/stream/http/async/client/stream_connection_handler.h"
#include "trpc/stream/http/async/server/stream_connection_handler.h"
#include "trpc/stream/http/http_client_stream_connection_handler.h"
#include "trpc/stream/trpc/trpc_client_stream_connection_handler.h"
#include "trpc/stream/trpc/trpc_server_stream_connection_handler.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler_factory.h"
#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler_factory.h"
#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler_factory.h"
#include "trpc/transport/server/default/server_connection_handler_factory.h"
#include "trpc/transport/server/fiber/fiber_server_connection_handler_factory.h"

namespace trpc {

bool InitConnectionHandler() {
  // Here we only register special ConnectionHandlers. If the codec cannot be found, the default ConnectionHandler
  // will be used as a fallback.
  // 1. For all protocols other than trpc/grpc, the framework uses the default ConnectionHandler.
  // 2. For business-customized protocols that do not require special ConnectionHandler processing, they all use the
  // default ConnectionHandler.

  // Registers fiber connection handler which used by server
  bool register_ret = FiberServerConnectionHandlerFactory::GetInstance()->Register(
      "trpc", [](Connection* c, FiberBindAdapter* a, BindInfo* i) {
        return std::make_unique<stream::FiberTrpcServerStreamConnectionHandler>(c, a, i);
      });
  TRPC_ASSERT(register_ret && "Register trpc server connection handler failed at fiber mode");

  register_ret = FiberServerConnectionHandlerFactory::GetInstance()->Register(
      "grpc", [](Connection* c, FiberBindAdapter* a, BindInfo* i) {
        return std::make_unique<stream::FiberGrpcServerStreamConnectionHandler>(c, a, i);
      });
  TRPC_ASSERT(register_ret && "Register grpc server connection handler failed at fiber mode");

  // Registers fiber connection handler which used by client.
  register_ret = FiberClientConnectionHandlerFactory::GetInstance()->Register("trpc", [](Connection* c, TransInfo* t) {
    return std::make_unique<stream::FiberTrpcClientStreamConnectionHandler>(c, t);
  });
  TRPC_ASSERT(register_ret && "Register trpc client connection handler failed at fiber mode");

  register_ret = FiberClientConnectionHandlerFactory::GetInstance()->Register("grpc", [](Connection* c, TransInfo* t) {
    return std::make_unique<stream::FiberGrpcClientStreamConnectionHandler>(c, t);
  });
  TRPC_ASSERT(register_ret && "Register http client connection handler failed at fiber mode");

  register_ret = FiberClientConnectionHandlerFactory::GetInstance()->Register("http", [](Connection* c, TransInfo* t) {
    return std::make_unique<stream::FiberHttpClientStreamConnectionHandler>(c, t);
  });
  TRPC_ASSERT(register_ret && "Register http client connection handler failed at fiber mode");

  register_ret = FiberClientConnectionHandlerFactory::GetInstance()->Register("http_sse", [](Connection* c, TransInfo* t) {
    return std::make_unique<stream::FiberHttpClientStreamConnectionHandler>(c, t);
  });
  TRPC_ASSERT(register_ret && "Register http_sse client connection handler failed at fiber mode");

  // Registers default connection handler which used by server in separate/merge threadmodel
  register_ret = DefaultServerConnectionHandlerFactory::GetInstance()->Register(
      "trpc", [](Connection* c, BindAdapter* a, BindInfo* i) {
        return std::make_unique<stream::DefaultTrpcServerStreamConnectionHandler>(c, a, i);
      });
  TRPC_ASSERT(register_ret && "Register trpc server connection handler failed at default mode");

  register_ret = DefaultServerConnectionHandlerFactory::GetInstance()->Register(
      "grpc", [](Connection* c, BindAdapter* a, BindInfo* i) {
        return std::make_unique<stream::DefaultGrpcServerStreamConnectionHandler>(c, a, i);
      });
  TRPC_ASSERT(register_ret && "Register trpc server connection handler failed at default mode");

  register_ret = DefaultServerConnectionHandlerFactory::GetInstance()->Register(
      "http", [](Connection* c, BindAdapter* a, BindInfo* i) {
        return std::make_unique<stream::DefaultHttpServerStreamConnectionHandler>(c, a, i);
      });
  TRPC_ASSERT(register_ret && "Register http server connection handler failed at default mode");

  register_ret = DefaultServerConnectionHandlerFactory::GetInstance()->Register(
      "http_sse", [](Connection* c, BindAdapter* a, BindInfo* i) {
        return std::make_unique<stream::DefaultHttpServerStreamConnectionHandler>(c, a, i);
      });
  TRPC_ASSERT(register_ret && "Register http_sse server connection handler failed at default mode");

  // Registers default connection handler which used by client in separate/merge threadmodel
  // 1. For conn_complex.
  register_ret = FutureConnComplexConnectionHandlerFactory::GetIntance()->Register(
      "trpc", [](const FutureConnectorOptions& options, FutureConnComplexMessageTimeoutHandler& handler) {
        return std::make_unique<stream::FutureTrpcClientStreamConnComplexConnectionHandler>(options, handler);
      });
  TRPC_ASSERT(register_ret && "Register trpc client connection handler failed at default mode(use conn_complex)");

  register_ret = FutureConnComplexConnectionHandlerFactory::GetIntance()->Register(
      "grpc", [](const FutureConnectorOptions& options, FutureConnComplexMessageTimeoutHandler& handler) {
        return std::make_unique<stream::FutureGrpcClientStreamConnComplexConnectionHandler>(options, handler);
      });
  TRPC_ASSERT(register_ret && "Register grpc client connection handler failed at default mode(use conn_complex)");

  // 2. For conn_pool.
  // Note: For trpc streaming in conn_pool:
  // Using TRPC streaming under connection pooling doesn't seem as urgent as connection reuse.
  // It is recommended to use TRPC streaming under connection reuse.

  register_ret = FutureConnPoolConnectionHandlerFactory::GetIntance()->Register(
      "grpc", [](const FutureConnectorOptions& options, FutureConnPoolMessageTimeoutHandler& handler) {
        return std::make_unique<stream::FutureGrpcClientStreamConnPoolConnectionHandler>(options, handler);
      });
  TRPC_ASSERT(register_ret && "Register grpc client connection handler failed at default mode(use conn_pool)");

  register_ret = FutureConnPoolConnectionHandlerFactory::GetIntance()->Register(
      "http", [](const FutureConnectorOptions& options, FutureConnPoolMessageTimeoutHandler& handler) {
        return std::make_unique<stream::HttpClientAsyncStreamConnectionHandler>(options, handler);
      });
  TRPC_ASSERT(register_ret && "Register http client connection handler failed at default mode(use conn_pool)");

  register_ret = FutureConnPoolConnectionHandlerFactory::GetIntance()->Register(
      "http_sse", [](const FutureConnectorOptions& options, FutureConnPoolMessageTimeoutHandler& handler) {
        return std::make_unique<stream::HttpClientAsyncStreamConnectionHandler>(options, handler);
      });
  TRPC_ASSERT(register_ret && "Register http_sse client connection handler failed at default mode(use conn_pool)");

  return register_ret;
}

void DestroyConnectionHandler() {
  FiberServerConnectionHandlerFactory::GetInstance()->Clear();
  DefaultServerConnectionHandlerFactory::GetInstance()->Clear();
  FiberClientConnectionHandlerFactory::GetInstance()->Clear();
  FutureConnComplexConnectionHandlerFactory::GetIntance()->Clear();
  FutureConnPoolConnectionHandlerFactory::GetIntance()->Clear();
}

}  // namespace trpc
