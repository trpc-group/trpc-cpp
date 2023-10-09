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

#include "trpc/stream/http/http_stream.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/http_service.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/transport/server/testing/server_transport_testing.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc::testing {

TEST(HttpReadStreamTest, ReadInBlockingMode) {
  RunAsFiber([&]() {
    http::Request request;
    size_t capacity = 1000;
    stream::HttpReadStream read_stream(&request, capacity, true);
    ASSERT_TRUE(read_stream.IsBlocking());

    // 1. testing with no data available
    ASSERT_EQ(capacity, read_stream.Capacity());
    read_stream.SetCapacity(++capacity);
    ASSERT_EQ(capacity, read_stream.Capacity());
    ASSERT_EQ(0, read_stream.Size());
    size_t read_all = std::numeric_limits<size_t>::max();
    NoncontiguousBuffer out;
    ASSERT_EQ(stream::kStreamStatusServerReadTimeout.GetFrameworkRetCode(),
              read_stream.Read(out, read_all, std::chrono::milliseconds(1)).GetFrameworkRetCode());

    // 2. testing with first writing data and then using the Read interface to read data
    NoncontiguousBuffer in = CreateBufferSlow("hello");
    read_stream.Write(std::move(in));
    ASSERT_EQ(5, read_stream.Size());
    in = CreateBufferSlow("world");
    read_stream.Write(std::move(in));
    ASSERT_EQ(10, read_stream.Size());
    ASSERT_TRUE(read_stream.Read(out, 5, std::chrono::milliseconds(1)).OK());
    ASSERT_EQ("hello", FlattenSlow(out));
    ASSERT_EQ(5, read_stream.Size());
    ASSERT_TRUE(read_stream.Read(out, 5, std::chrono::milliseconds(1)).OK());
    ASSERT_EQ("world", FlattenSlow(out));
    ASSERT_EQ(0, read_stream.Size());
    ASSERT_EQ(stream::kStreamStatusServerReadTimeout.GetFrameworkRetCode(),
              read_stream.Read(out, read_all, std::chrono::milliseconds(1)).GetFrameworkRetCode());

    // 3. testing to read until the end of the stream
    in = CreateBufferSlow("helloworld");
    size_t real_size = in.ByteSize();
    read_stream.Write(std::move(in));
    ASSERT_EQ(real_size, read_stream.Size());
    // since there is no data of expected_size bytes, the read operation timed out
    size_t expected_size = real_size + 1;
    ASSERT_EQ(stream::kStreamStatusServerReadTimeout.GetFrameworkRetCode(),
              read_stream.Read(out, expected_size, std::chrono::milliseconds(1)).GetFrameworkRetCode());
    read_stream.WriteEof();
    // since the stream has already reached the end, there is no data of expected_size bytes, but the operation
    // returns success
    ASSERT_TRUE(read_stream.Read(out, expected_size, std::chrono::milliseconds(1)).OK());
    ASSERT_LT(out.ByteSize(), expected_size);
    ASSERT_EQ("helloworld", FlattenSlow(out));
    // attempting to read again will return kStreamStatusReadEof
    ASSERT_EQ(stream::kStreamStatusReadEof.GetFrameworkRetCode(),
              read_stream.Read(out, read_all, std::chrono::milliseconds(1)).GetFrameworkRetCode());
  });
}

TEST(HttpReadStreamTest, ReadAllInBlockingMode) {
  RunAsFiber([&]() {
    http::Request request;
    size_t capacity = 1000;
    stream::HttpReadStream read_stream(&request, capacity, true);
    ASSERT_TRUE(read_stream.IsBlocking());

    NoncontiguousBuffer in = CreateBufferSlow("helloworld");
    read_stream.Write(std::move(in));
    ASSERT_EQ(10, read_stream.Size());
    NoncontiguousBuffer out;
    // since the end of the stream has not been reached yet, the operation returns timeout
    ASSERT_EQ(stream::kStreamStatusServerReadTimeout.GetFrameworkRetCode(),
              read_stream.ReadAll(out, std::chrono::milliseconds(1)).GetFrameworkRetCode());

    // since the end of the stream has been reached, the operation returns success
    read_stream.WriteEof();
    ASSERT_TRUE(read_stream.ReadAll(out, std::chrono::milliseconds(1)).OK());
    ASSERT_EQ("helloworld", FlattenSlow(out));
  });
}

TEST(HttpReadStreamTest, AppendToRequestInBlockingMode) {
  RunAsFiber([&]() {
    http::Request request;
    size_t capacity = 1000;
    stream::HttpReadStream read_stream(&request, capacity, true);
    ASSERT_TRUE(read_stream.IsBlocking());

    NoncontiguousBuffer in = CreateBufferSlow("helloworld");
    read_stream.Write(std::move(in));
    read_stream.WriteEof();
    // because the AppendToRequest interface was not called, the data is saved in the queue and there is no data in
    // the request
    ASSERT_EQ(10, read_stream.Size());
    ASSERT_EQ(0, request.GetNonContiguousBufferContent().ByteSize());

    // after calling the AppendToRequest interface, all data is transferred to the request
    ASSERT_TRUE(read_stream.AppendToRequest(capacity).OK());
    ASSERT_EQ(0, read_stream.Size());
    ASSERT_EQ(10, request.GetNonContiguousBufferContent().ByteSize());
    ASSERT_EQ("helloworld", FlattenSlow(request.GetNonContiguousBufferContent()));

    // attempting to read again will return kStreamStatusReadEof
    NoncontiguousBuffer out;
    ASSERT_EQ(stream::kStreamStatusReadEof.GetFrameworkRetCode(),
              read_stream.Read(out, 1, std::chrono::milliseconds(1)).GetFrameworkRetCode());

    read_stream.Close();
  });
}

TEST(HttpReadStreamTest, DefaultMode) {
  http::Request request;
  size_t capacity = 1000;
  stream::HttpReadStream read_stream(&request, capacity, false);

  ASSERT_FALSE(read_stream.IsBlocking());

  // 1. testing with no data available
  ASSERT_EQ(capacity, read_stream.Capacity());
  ASSERT_EQ(0, read_stream.Size());
  ASSERT_EQ(0, request.GetNonContiguousBufferContent().ByteSize());

  // 2. testing with writing data
  NoncontiguousBuffer in = CreateBufferSlow("hello");
  read_stream.Write(std::move(in));
  ASSERT_EQ(5, read_stream.Size());
  in = CreateBufferSlow("world");
  read_stream.Write(std::move(in));
  ASSERT_EQ(10, read_stream.Size());
  // the data stores in request
  ASSERT_EQ(10, request.GetNonContiguousBufferContent().ByteSize());

  // end of stream
  read_stream.WriteEof();
  ASSERT_EQ(10, request.GetNonContiguousBufferContent().ByteSize());

  read_stream.Close();
}

class HttpWriteStreamTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    codec::Init();
    serialization::Init();
  }

  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
  }
};

TEST_F(HttpWriteStreamTest, WriteHeader) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection connection;
  context->SetReserved(&connection);
  http::Response response;
  stream::HttpWriteStream write_stream(&response, context.get());

  size_t capacity = 1000;
  write_stream.SetCapacity(capacity);
  ASSERT_EQ(capacity, write_stream.Capacity());

  // 1. testing response 100 Continue
  response.SetStatus(http::Response::StatusCode::kContinue);
  ASSERT_TRUE(write_stream.WriteHeader().OK());
  ASSERT_EQ(http::Response::StatusCode::kOk, response.GetStatus());

  // 2. testing response other headers
  response.SetHeader("header1", "1");
  response.SetHeader("header2", "2");
  ASSERT_TRUE(write_stream.WriteHeader().OK());

  write_stream.Close();
}

TEST_F(HttpWriteStreamTest, WriteForChunkedResponse) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection connection;
  context->SetReserved(&connection);
  // do not set the Content-Length, represents that the response is chunked response
  http::Response response;
  response.SetStatus(http::Response::StatusCode::kOk);
  stream::HttpWriteStream write_stream(&response, context.get());
  write_stream.SetCapacity(1000);

  // write header first
  ASSERT_TRUE(write_stream.WriteHeader().OK());
  // write data
  NoncontiguousBuffer in = CreateBufferSlow("hello");
  ASSERT_TRUE(write_stream.Write(std::move(in)).OK());
  in = CreateBufferSlow("world");
  ASSERT_TRUE(write_stream.Write(std::move(in)).OK());
  ASSERT_TRUE(write_stream.WriteDone().OK());

  write_stream.Close();
}

TEST_F(HttpWriteStreamTest, WriteForNonChunkedResponse) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection connection;
  context->SetReserved(&connection);
  http::Response response;
  response.SetStatus(http::Response::StatusCode::kOk);
  // set Content-Length, represents that the response is non-chunked response
  response.SetHeader(http::kHeaderContentLength, "10");
  stream::HttpWriteStream write_stream(&response, context.get());
  write_stream.SetCapacity(1000);

  // write header first
  ASSERT_TRUE(write_stream.WriteHeader().OK());

  // write fist block, the length to be sent do not exceed the remaining length
  NoncontiguousBuffer in = CreateBufferSlow("hello");
  ASSERT_TRUE(write_stream.Write(std::move(in)).OK());

  // try to write second, the length to be sent exceeds the remaining length
  in = CreateBufferSlow("helloworld");
  ASSERT_EQ(stream::kStreamStatusServerWriteContentLengthError.GetFrameworkRetCode(),
            write_stream.Write(std::move(in)).GetFrameworkRetCode());

  // correctly write the remaining length of data
  in = CreateBufferSlow("world");
  ASSERT_TRUE(write_stream.Write(std::move(in)).OK());
  ASSERT_TRUE(write_stream.WriteDone().OK());

  write_stream.Close();
}

}  // namespace trpc::testing
