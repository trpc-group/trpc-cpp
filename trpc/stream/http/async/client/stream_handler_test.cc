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

#include "trpc/stream/http/async/client/stream_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"

namespace trpc::testing {

using namespace trpc::stream;

TEST(HttpClientAsyncStreamHandlerTest, TestCreateStream) {
  StreamOptions stream_handler_options;
  auto stream_handler = MakeRefCounted<HttpClientAsyncStreamHandler>(std::move(stream_handler_options));

  StreamOptions options;
  options.stream_handler = stream_handler;
  options.context.context = MakeRefCounted<ClientContext>();
  ASSERT_TRUE(stream_handler->CreateStream(std::move(options)) != nullptr);

  // When the stream exists, creating a new one will fail. Currently, only one stream can exist on a connection.
  ASSERT_TRUE(stream_handler->CreateStream(StreamOptions{}) == nullptr);

  // When the connection is about to be closed, a new stream cannot be created.
  stream_handler->Stop();
  // Stream was removed.
  ASSERT_TRUE(stream_handler->IsNewStream(0, 0));
  // The stream creation failed because the network has already been closed.
  ASSERT_TRUE(stream_handler->CreateStream(StreamOptions{}) == nullptr);
}

}  // namespace trpc::testing
