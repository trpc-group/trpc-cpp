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

#include "trpc/codec/http/http_proto_checker.h"

#include <any>
#include <string>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/stream/http_client_stream_response.h"

namespace trpc::testing {

namespace {
class MockConnectionHandler : public ConnectionHandler {
  Connection* GetConnection() const override { return nullptr; }
  int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) override {
    return PacketChecker::PACKET_FULL;
  }

  bool HandleMessage(const ConnectionPtr&, std::deque<std::any>&) override { return true; }
};
}  // namespace

class HttpClientProtoCheckerZeroCopyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    in_.Clear();
    out_.clear();

    conn_ = MakeRefCounted<Connection>();
    conn_->SetConnType(ConnectionType::kTcpLong);
    conn_->SetConnectionHandler(std::make_unique<MockConnectionHandler>());
  }

  void TearDown() override {}

 public:
  ConnectionPtr conn_;
  NoncontiguousBuffer in_;
  std::deque<std::any> out_;
};

TEST_F(HttpClientProtoCheckerZeroCopyTest, EmptyPacket) {
  std::string s = "";
  NoncontiguousBufferBuilder builder{};

  TRPC_ASSERT(conn_ && "conn_ is nullptr");
  std::cout << "11" << std::endl;
  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();

  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

  EXPECT_EQ(r, PacketChecker::PACKET_LESS);
  EXPECT_EQ(out_.size(), 0);
  EXPECT_EQ(in_.ByteSize(), 0);
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, SingleFullPacket) {
  std::string s = "HTTP/1.1 200 OK\r\nContent-Length:6\r\n\r\n123456";
  NoncontiguousBufferBuilder builder{};

  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();

  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

  EXPECT_EQ(r, PacketChecker::PACKET_FULL);
  EXPECT_EQ(out_.size(), 1);
  EXPECT_EQ(in_.ByteSize(), 0);
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, SingleLessPacket) {
  std::string s = "HTTP/1.1 200 OK\r\nContent-Length:6\r\n\r\n1234";
  NoncontiguousBufferBuilder builder{};

  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();

  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

  // Completed response header.
  EXPECT_EQ(r, PacketChecker::PACKET_LESS);
  // Response will be checked out when both header and content are completed.
  EXPECT_EQ(out_.size(), 0);
  EXPECT_EQ(in_.ByteSize(), 0);
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, SingleBadPacket) {
  std::string s = "HTTP/1.1 2001 OK\r\nContent-Length:6\r\n\r\n1234";
  NoncontiguousBufferBuilder builder{};

  builder.Append(s.c_str(), s.size());
  in_ = builder.DestructiveGet();

  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

  EXPECT_EQ(r, PacketChecker::PACKET_ERR);
  EXPECT_EQ(out_.size(), 0);
  EXPECT_EQ(in_.ByteSize(), s.size());
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, MultiFullPacket) {
  std::string s = "HTTP/1.1 200 OK\r\nContent-Length:5\r\n\r\n12345";
  NoncontiguousBufferBuilder builder{};

  for (int i = 0; i < 3; ++i) {
    builder.Append(s.c_str(), s.size());
  }

  in_ = builder.DestructiveGet();

  for (int i = 0; i < 3; ++i) {
    auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);
    EXPECT_EQ(r, PacketChecker::PACKET_FULL);
  }
  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);
  EXPECT_EQ(r, PacketChecker::PACKET_LESS);

  EXPECT_EQ(out_.size(), 3);
  EXPECT_EQ(in_.ByteSize(), 0);
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, MultiNotFullPacket) {
  std::string s = "HTTP/1.1 200 OK\r\nContent-Length:5\r\n\r\n12345";
  NoncontiguousBufferBuilder builder{};

  for (int i = 0; i < 2; ++i) {
    builder.Append(s.c_str(), s.size());
  }

  builder.Append(s.c_str(), 10);
  in_ = builder.DestructiveGet();

  for (int i = 0; i < 2; ++i) {
    auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);
    EXPECT_EQ(r, PacketChecker::PACKET_FULL);
  }
  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);
  EXPECT_EQ(r, PacketChecker::PACKET_LESS);

  EXPECT_EQ(out_.size(), 2);
  EXPECT_EQ(in_.ByteSize(), 10);
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, CrossFullPacket) {
  std::string s = "HTTP/1.1 200 OK\r\nContent-Length:5\r\n\r\n12345";
  NoncontiguousBufferBuilder builder{};

  builder.Append(s.c_str(), 10);
  builder.Append(s.c_str() + 10, s.size() - 10);

  in_ = builder.DestructiveGet();

  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

  EXPECT_EQ(r, PacketChecker::PACKET_FULL);
  EXPECT_EQ(out_.size(), 1);
  EXPECT_EQ(in_.ByteSize(), 0);
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, Chunked) {
  std::string s =
      "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n"
      "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
  NoncontiguousBufferBuilder builder{};
  builder.Append(s);
  in_ = builder.DestructiveGet();

  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

  EXPECT_EQ(r, PacketChecker::PACKET_FULL);
  EXPECT_EQ(out_.size(), 1);
  EXPECT_EQ(in_.ByteSize(), 0);
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, ChunkedTransferEncodingHeader) {
  std::string s =
      "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n"
      "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
  NoncontiguousBufferBuilder builder{};
  builder.Append(s);
  in_ = builder.DestructiveGet();

  auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

  EXPECT_EQ(r, PacketChecker::PACKET_FULL);
  EXPECT_EQ(out_.size(), 1);
  EXPECT_EQ(in_.ByteSize(), 0);

  auto reply = std::any_cast<http::HttpResponse>(out_.front());
  auto found = reply.GetHeader(http::kHeaderTransferEncoding);
  EXPECT_TRUE(!found.empty());

  found = reply.GetHeader(http::kHeaderContentLength);
  EXPECT_TRUE(found.empty());
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, ParsePacketInflightOk) {
  std::string s =
      "HTTP/1.1 200 OK\r\n"
      "Content-Length:40\r\n"
      "\r\n"
      "1234567890";

  NoncontiguousBufferBuilder partial_content_builder_0{};
  partial_content_builder_0.Append(s);
  in_ = partial_content_builder_0.DestructiveGet();

  // There is uncompleted response, response message will be stored in buffer.
  auto result = HttpZeroCopyCheckResponse(conn_, in_, out_);
  ASSERT_EQ(result, PacketChecker::PACKET_LESS);
  ASSERT_EQ(out_.size(), 0);
  ASSERT_EQ(in_.ByteSize(), 0);

  // Continue to append content of the response.
  NoncontiguousBufferBuilder partial_content_builder_1{};
  partial_content_builder_1.Append("1234567890");
  in_.Append(partial_content_builder_1.DestructiveGet());
  result = HttpZeroCopyCheckResponse(conn_, in_, out_);

  ASSERT_EQ(result, PacketChecker::PACKET_LESS);
  ASSERT_EQ(out_.size(), 0);
  ASSERT_EQ(in_.ByteSize(), 0);

  NoncontiguousBufferBuilder partial_content_builder_2{};
  partial_content_builder_2.Append("12345678901234567890");
  in_.Append(partial_content_builder_2.DestructiveGet());
  result = HttpZeroCopyCheckResponse(conn_, in_, out_);

  ASSERT_EQ(result, PacketChecker::PACKET_FULL);
  ASSERT_EQ(out_.size(), 1);
  ASSERT_EQ(in_.ByteSize(), 0);

  out_.clear();
  // More an uncompleted response.
  NoncontiguousBufferBuilder partial_content_builder_3{};
  partial_content_builder_3.Append(s);
  in_.Append(partial_content_builder_3.DestructiveGet());

  NoncontiguousBufferBuilder partial_content_builder_5{};
  partial_content_builder_5.Append("123456789012345678901234567890");
  in_.Append(partial_content_builder_5.DestructiveGet());

  NoncontiguousBufferBuilder partial_content_builder_6{};
  partial_content_builder_6.Append(s);
  in_.Append(partial_content_builder_6.DestructiveGet());

  result = HttpZeroCopyCheckResponse(conn_, in_, out_);
  ASSERT_EQ(result, PacketChecker::PACKET_FULL);
  result = HttpZeroCopyCheckResponse(conn_, in_, out_);
  ASSERT_EQ(result, PacketChecker::PACKET_LESS);

  ASSERT_EQ(out_.size(), 1);
  ASSERT_EQ(in_.ByteSize(), 0);

  NoncontiguousBufferBuilder partial_content_builder_7{};
  partial_content_builder_7.Append("123456789012345678901234567890");
  in_.Append(partial_content_builder_7.DestructiveGet());

  std::string partial_header =
      "HTTP/1.1 200 OK\r\n"
      "Content-Length:40\r\n";
  NoncontiguousBufferBuilder partial_content_builder_8{};
  partial_content_builder_8.Append(partial_header);
  in_.Append(partial_content_builder_8.DestructiveGet());

  result = HttpZeroCopyCheckResponse(conn_, in_, out_);
  ASSERT_EQ(result, PacketChecker::PACKET_FULL);
  ASSERT_EQ(out_.size(), 2);
  // There is an uncompleted response header in buffer.
  ASSERT_EQ(in_.ByteSize(), partial_header.size());
}

TEST_F(HttpClientProtoCheckerZeroCopyTest, OnStream) {
  RunAsFiber([&]() {
    stream::StreamOptions stream_options;
    ClientContextPtr client_context = MakeRefCounted<ClientContext>();
    client_context->SetTimeout(1);
    stream_options.context.context = client_context;
    stream_options.callbacks.on_close_cb = [](int reason) {};
    auto stream = MakeRefCounted<stream::HttpClientStream>(std::move(stream_options));
    auto response = std::make_shared<stream::HttpClientStreamResponse>();
    response->GetStream() = stream;
    conn_->SetUserAny(std::move(response));

    std::string s =
        "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n"
        "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
    NoncontiguousBufferBuilder builder{};
    builder.Append(s);
    in_ = builder.DestructiveGet();

    auto r = HttpZeroCopyCheckResponse(conn_, in_, out_);

    EXPECT_EQ(r, PacketChecker::PACKET_FULL);
    EXPECT_EQ(out_.size(), 1);
    EXPECT_EQ(in_.ByteSize(), 0);

    auto reply = std::any_cast<http::HttpResponse>(out_.front());
    auto found = reply.GetHeader(http::kHeaderTransferEncoding);
    EXPECT_TRUE(!found.empty());

    EXPECT_EQ(stream->Size(), 5 + 5);

    conn_->GetUserAny() = nullptr;
  });
}

}  // namespace trpc::testing
