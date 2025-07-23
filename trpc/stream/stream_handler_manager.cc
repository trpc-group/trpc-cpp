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

#include "trpc/stream/stream_handler_manager.h"

#include <memory>
#include <utility>

#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/grpc/grpc_client_stream_handler.h"
#include "trpc/stream/grpc/grpc_server_stream_handler.h"
#include "trpc/stream/http/async/client/stream_handler.h"
#include "trpc/stream/http/async/server/stream_handler.h"
#include "trpc/stream/http/http_client_stream_handler.h"
#include "trpc/stream/server_stream_handler_factory.h"
#include "trpc/stream/trpc/trpc_client_stream_handler.h"
#include "trpc/stream/trpc/trpc_server_stream_handler.h"

namespace trpc::stream {

bool InitStreamHandler() {
  // tRPC.
  ServerStreamHandlerFactory::GetInstance()->Register(
      "trpc", [](StreamOptions&& options) { return MakeRefCounted<TrpcServerStreamHandler>(std::move(options)); });
  ClientStreamHandlerFactory::GetInstance()->Register(
      "trpc", [](StreamOptions&& options) { return MakeRefCounted<TrpcClientStreamHandler>(std::move(options)); });

  // HTTP.
  ServerStreamHandlerFactory::GetInstance()->Register("http", [](StreamOptions&& options) {
    StreamHandlerPtr stream_handler;
    if (options.fiber_mode) {
      // HTTP synchronous server stream was implemented in other way instead of HTTP stream handler.
      stream_handler = nullptr;
    } else {
      stream_handler = MakeRefCounted<HttpServerAsyncStreamHandler>(std::move(options));
    }
    return stream_handler;
  });
  ClientStreamHandlerFactory::GetInstance()->Register("http", [](StreamOptions&& options) {
    StreamHandlerPtr stream_handler;
    if (options.fiber_mode) {
      stream_handler = MakeRefCounted<HttpClientStreamHandler>(std::move(options));
    } else {
      stream_handler = MakeRefCounted<HttpClientAsyncStreamHandler>(std::move(options));
    }
    return stream_handler;
  });

  // gRPC.
  ServerStreamHandlerFactory::GetInstance()->Register("grpc", [](StreamOptions&& options) {
    StreamHandlerPtr stream_handler;
    if (options.fiber_mode) {
      stream_handler = MakeRefCounted<FiberGrpcServerStreamHandler>(std::move(options));
    } else {
      stream_handler = MakeRefCounted<DefaultGrpcServerStreamHandler>(std::move(options));
    }
    return stream_handler;
  });
  ClientStreamHandlerFactory::GetInstance()->Register("grpc", [](StreamOptions&& options) {
    StreamHandlerPtr stream_handler;
    if (options.fiber_mode) {
      stream_handler = MakeRefCounted<GrpcFiberClientStreamHandler>(std::move(options));
    } else {
      stream_handler = MakeRefCounted<GrpcDefaultClientStreamHandler>(std::move(options));
    }
    return stream_handler;
  });

  return true;
}

void DestroyStreamHandler() {
  ServerStreamHandlerFactory::GetInstance()->Clear();
  ClientStreamHandlerFactory::GetInstance()->Clear();
}

}  // namespace trpc::stream
