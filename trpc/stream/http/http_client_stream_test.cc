//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/stream/http/http_client_stream.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/http/http_client_stream_handler.h"

namespace trpc::testing {

namespace {

stream::HttpClientStreamPtr GetClientStream() {
  stream::StreamOptions handler_options;
  handler_options.send = [](IoMessage&& message) { return 0; };
  auto handler = MakeRefCounted<stream::HttpClientStreamHandler>(std::move(handler_options));

  stream::StreamOptions stream_options;
  stream_options.stream_handler = handler;
  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  client_context->SetTimeout(1);
  stream_options.context.context = client_context;
  stream_options.callbacks.on_close_cb = [](int reason) {};
  return MakeRefCounted<stream::HttpClientStream>(std::move(stream_options));
}

}  // namespace

TEST(HttpClientStreamTest, TestProvider) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    ASSERT_TRUE(std::any_cast<const ClientContextPtr>(stream->GetMutableStreamOptions()->context.context));

    // No data.
    size_t capacity = 1000;
    stream->SetCapacity(capacity);
    ASSERT_EQ(capacity, stream->Capacity());
    ASSERT_EQ(0, stream->Size());
    int code = 0;
    http::HttpHeader http_header;
    ASSERT_EQ(stream::kStreamStatusClientReadTimeout.GetFrameworkRetCode(),
              stream->ReadHeaders(code, http_header).GetFrameworkRetCode());
    NoncontiguousBuffer out;
    ASSERT_EQ(stream::kStreamStatusClientReadTimeout.GetFrameworkRetCode(),
              stream->Read(out, 100).GetFrameworkRetCode());

    // Sends HTTP request header.
    HttpRequestProtocol protocol{std::make_shared<http::Request>()};
    protocol.request->SetHeader(http::kHeaderContentLength, "5");
    stream->SetHttpRequestProtocol(&protocol);
    stream->SetMethod(http::OperationType::PUT);
    ASSERT_TRUE(stream->SendRequestHeader().OK());

    // Receives content.
    http::HttpResponse http_response;
    http_response.SetStatus(200);
    http_response.AddHeader("Content-Type", "application/json");
    stream->PushRecvMessage(std::move(http_response));
    NoncontiguousBuffer in = CreateBufferSlow("hello");
    stream->PushDataToRecvQueue(std::move(in));
    ASSERT_EQ(5, stream->Size());
    in = CreateBufferSlow("world");
    stream->PushDataToRecvQueue(std::move(in));
    ASSERT_EQ(10, stream->Size());

    ASSERT_TRUE(stream->ReadHeaders(code, http_header).OK());
    ASSERT_EQ(200, code);
    ASSERT_EQ("application/json", http_header.Get("Content-Type"));
    ASSERT_TRUE(stream->Read(out, 6).OK());
    ASSERT_EQ("hellow", FlattenSlow(out));
    ASSERT_EQ(4, stream->Size());

    // Receives EOF.
    stream->PushEofToRecvQueue();
    ASSERT_TRUE(stream->ReadAll(out).OK());
    ASSERT_EQ("orld", FlattenSlow(out));
    ASSERT_EQ(stream::kStreamStatusReadEof.GetFrameworkRetCode(), stream->Read(out, 100).GetFrameworkRetCode());

    // Sends content.
    in = CreateBufferSlow("hello");
    ASSERT_TRUE(stream->Write(std::move(in)).OK());
    ASSERT_TRUE(stream->WriteDone().OK());

    stream->Close();
  });
}

TEST(HttpClientStreamTest, TestProviderClose) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    ASSERT_TRUE(std::any_cast<const ClientContextPtr>(stream->GetMutableStreamOptions()->context.context));

    // Sends HTTP request header. The inner state will transfer to kReading
    size_t capacity = 1000;
    stream->SetCapacity(capacity);
    HttpRequestProtocol protocol{std::make_shared<http::Request>()};
    protocol.request->SetHeader(http::kHeaderContentLength, "10");
    stream->SetHttpRequestProtocol(&protocol);
    stream->SetMethod(http::OperationType::PUT);
    ASSERT_TRUE(stream->SendRequestHeader().OK());

    // Receives EOF.
    NoncontiguousBuffer in = CreateBufferSlow("helloworld");
    stream->PushDataToRecvQueue(std::move(in));
    ASSERT_EQ(10, stream->Size());
    stream->PushEofToRecvQueue();

    // Stream not closed, reading is normal.
    NoncontiguousBuffer out1;
    ASSERT_TRUE(stream->Read(out1, 5).OK());
    ASSERT_EQ("hello", FlattenSlow(out1));
    ASSERT_EQ(5, stream->Size());

    // Stream closed, reading should still be normal.
    NoncontiguousBuffer out2;
    stream->Close();
    ASSERT_TRUE(stream->Read(out2, 5).OK());
    ASSERT_EQ("world", FlattenSlow(out2));
    ASSERT_EQ(0, stream->Size());
  });
}

TEST(HttpClientStreamTest, CreateStreamReaderWriter) {
  RunAsFiber([&]() {
    bool closing = true;
    stream::HttpClientStreamReaderWriter StreamReaderWriter =
        Create(MakeRefCounted<stream::HttpClientStream>(stream::kStreamStatusClientNetworkError, closing));

    int code;
    http::HttpHeader http_header;
    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.ReadHeaders(code, http_header).GetFrameworkRetCode());

    NoncontiguousBuffer out;
    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.Read(out, 100).GetFrameworkRetCode());

    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.ReadAll(out).GetFrameworkRetCode());

    NoncontiguousBuffer in;
    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.Write(std::move(in)).GetFrameworkRetCode());

    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.WriteDone().GetFrameworkRetCode());

    StreamReaderWriter.Close();
  });
}

}  // namespace trpc::testing
