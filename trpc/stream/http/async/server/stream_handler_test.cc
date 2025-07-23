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

#include "trpc/stream/http/async/server/stream_handler.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/http/http_server_codec.h"

namespace trpc::testing {

using namespace trpc::stream;

TEST(HttpServerAsyncStreamHandlerTest, TestCreateStream) {
  trpc::codec::Init();

  StreamOptions stream_handler_options;
  auto stream_handler = MakeRefCounted<HttpServerAsyncStreamHandler>(std::move(stream_handler_options));

  StreamOptions options;
  options.stream_handler = stream_handler;
  options.context.context = MakeRefCounted<ServerContext>();
  ;
  ASSERT_NE(stream_handler->CreateStream(std::move(options)), nullptr);

  // Currently, only one stream can exist per connection, and attempting to create another stream while one already
  // exists will fail.
  ASSERT_EQ(stream_handler->CreateStream(StreamOptions{}), nullptr);

  // can not create stream when a connection is being closed
  stream_handler->Stop();
  ASSERT_TRUE(stream_handler->IsNewStream(0, 0));
  ASSERT_EQ(stream_handler->CreateStream(StreamOptions{}), nullptr);

  trpc::codec::Destroy();
}

}  // namespace trpc::testing
