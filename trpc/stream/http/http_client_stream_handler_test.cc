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

#include "trpc/stream/http/http_client_stream_handler.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc::testing {

TEST(HttpClientStreamHandlerTest, Run) {
  RunAsFiber([&]() {
    FiberLatch fiber_latch(1);

    StartFiberDetached([&]() {
      stream::StreamOptions options;
      options.send = [](IoMessage&& message) { return 0; };
      stream::HttpClientStreamHandler handler(std::move(options));
      EXPECT_TRUE(handler.Init());

      auto ctx = MakeRefCounted<ClientContext>();
      stream::StreamOptions options2;
      options2.context.context = ctx;
      auto stream = handler.CreateStream(std::move(options2));
      EXPECT_TRUE(stream);
      EXPECT_TRUE(handler.GetHttpStream());

      NoncontiguousBuffer in = CreateBufferSlow("hello");
      EXPECT_EQ(0, handler.SendMessage(ctx, std::move(in)));

      // Pushes the response header to stream.
      http::HttpResponse http_response;
      handler.PushMessage(std::move(http_response), stream::ProtocolMessageMetadata{});

      EXPECT_FALSE(handler.IsNewStream(0, 0));
      handler.Stop();
      EXPECT_EQ(0, handler.RemoveStream(0));

      fiber_latch.CountDown();
    });

    fiber_latch.Wait();
  });
}

}  // namespace trpc::testing
