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

#include <utility>

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/stream/client_stream_handler_factory.h"
#include "trpc/stream/server_stream_handler_factory.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

TEST(StreamHandlerManagerTest, InitOk) {
  trpc::codec::Init();

  ASSERT_TRUE(InitStreamHandler());

  auto trpc_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("trpc", std::move(StreamOptions{}));
  ASSERT_TRUE(trpc_client_stream_handler);

  StreamOptions trpc_client_stream_options{};
  trpc_client_stream_options.fiber_mode = true;
  trpc_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("trpc", std::move(trpc_client_stream_options));
  ASSERT_TRUE(trpc_client_stream_handler);

  auto trpc_server_stream_handler =
      ServerStreamHandlerFactory::GetInstance()->Create("trpc", std::move(StreamOptions{}));
  ASSERT_TRUE(trpc_server_stream_handler);

  StreamOptions trpc_server_stream_options{};
  trpc_server_stream_options.fiber_mode = true;
  trpc_server_stream_handler =
      ServerStreamHandlerFactory::GetInstance()->Create("trpc", std::move(trpc_server_stream_options));
  ASSERT_TRUE(trpc_server_stream_handler);

  auto grpc_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("grpc", std::move(StreamOptions{}));
  ASSERT_TRUE(grpc_client_stream_handler);

  StreamOptions grpc_client_stream_options;
  grpc_client_stream_options.fiber_mode = true;
  grpc_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("grpc", std::move(grpc_client_stream_options));
  ASSERT_TRUE(grpc_client_stream_handler);

  auto grpc_server_stream_handler =
      ServerStreamHandlerFactory::GetInstance()->Create("grpc", std::move(StreamOptions{}));
  ASSERT_TRUE(grpc_server_stream_handler);

  StreamOptions grpc_server_stream_options{};
  grpc_server_stream_options.fiber_mode = true;
  grpc_server_stream_handler =
      ServerStreamHandlerFactory::GetInstance()->Create("grpc", std::move(grpc_server_stream_options));
  ASSERT_TRUE(grpc_server_stream_handler);

  auto http_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("http", std::move(StreamOptions{}));
  ASSERT_TRUE(http_client_stream_handler);

  StreamOptions http_client_stream_options;
  http_client_stream_options.fiber_mode = true;
  http_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("http", std::move(http_client_stream_options));
  ASSERT_TRUE(http_client_stream_handler);
}

TEST(StreamHandlerManagerTest, DestroyOk) {
  DestroyStreamHandler();

  StreamOptions trpc_client_stream_options{};
  trpc_client_stream_options.fiber_mode = true;
  auto trpc_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("trpc", std::move(trpc_client_stream_options));
  ASSERT_FALSE(trpc_client_stream_handler);

  StreamOptions trpc_server_stream_options{};
  trpc_server_stream_options.fiber_mode = true;
  auto trpc_server_stream_handler =
      ServerStreamHandlerFactory::GetInstance()->Create("trpc", std::move(trpc_server_stream_options));
  ASSERT_FALSE(trpc_server_stream_handler);

  auto grpc_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("grpc", std::move(StreamOptions{}));
  ASSERT_FALSE(grpc_client_stream_handler);

  auto grpc_server_stream_handler =
      ServerStreamHandlerFactory::GetInstance()->Create("grpc", std::move(StreamOptions{}));
  ASSERT_FALSE(grpc_server_stream_handler);

  StreamOptions http_client_stream_options{};
  http_client_stream_options.fiber_mode = true;
  auto http_client_stream_handler =
      ClientStreamHandlerFactory::GetInstance()->Create("http", std::move(http_client_stream_options));
  ASSERT_FALSE(http_client_stream_handler);

  trpc::codec::Destroy();
}

}  // namespace trpc::testing
