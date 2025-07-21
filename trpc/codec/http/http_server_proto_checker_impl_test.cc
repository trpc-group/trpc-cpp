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

#include "trpc/codec/http/http_proto_checker.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/util/http/request.h"

namespace trpc::testing {

class HttpServerProtoCheckerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    in_.Clear();
    out_.clear();

    conn_ = MakeRefCounted<Connection>();
    conn_->SetConnType(ConnectionType::kTcpLong);
  }

  void TearDown() override {}

 protected:
  ConnectionPtr conn_;
  NoncontiguousBuffer in_;
  std::deque<std::any> out_;
};

TEST_F(HttpServerProtoCheckerTest, EmptyPacket) {
  std::string s = "";
  in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

  ASSERT_EQ(kPacketLess, result);
  ASSERT_EQ(0, out_.size());
}

TEST_F(HttpServerProtoCheckerTest, NotFullPacket) {
  std::string s = "POST /form.html HTTP/1.1\r\n";
  in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

  ASSERT_EQ(kPacketLess, result);
  ASSERT_EQ(0, out_.size());
  ASSERT_EQ(s.size(), in_.ByteSize());
}

TEST_F(HttpServerProtoCheckerTest, ErrorPacket) {
  // the header format is incorrect
  std::string s = "POST /form.html HTTP/1.1\rContent-Length:5\r\ncontent-type:text/html\r\n\r\nhello";
  in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

  ASSERT_EQ(kPacketError, result);
  ASSERT_EQ(0, out_.size());
}

TEST_F(HttpServerProtoCheckerTest, DefaultSingleNonChunkedFullPacket1) {
  // the header and the body is complete
  std::string s = "POST /form.html HTTP/1.1\r\nContent-Length:5\r\ncontent-type:text/html\r\n\r\nhello";
  in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

  ASSERT_EQ(kPacketFull, result);
  ASSERT_EQ(1, out_.size());
  ASSERT_EQ(0, in_.ByteSize());

  const auto& req = std::any_cast<const http::RequestPtr&>(out_.front());
  ASSERT_TRUE(req->IsPost());
  ASSERT_EQ("text/html", req->GetHeader("Content-Type"));
  ASSERT_TRUE(req->GetStream().AppendToRequest(5).OK());
  ASSERT_EQ(5, req->ContentLength());
  ASSERT_EQ("hello", FlattenSlow(req->GetNonContiguousBufferContent()));
}

TEST_F(HttpServerProtoCheckerTest, DefaultSingleNonChunkedFullPacket2) {
  // the header is complete, but the body is incomplete
  std::string s = "POST /form.html HTTP/1.1\r\nContent-Length:10\r\ncontent-type:text/html\r\n\r\nhello";
  in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

  // In non-blocking mode, if the header is complete but the body is incomplete, an intermediate request will be placed
  // into the connection. It will remain in the connection until a subsequent check when the body is fully completed, at
  // which point it will be placed in the out queue.
  ASSERT_EQ(kPacketLess, result);
  ASSERT_EQ(0, out_.size());
  ASSERT_EQ(0, in_.ByteSize());

  // wait until the body is complete
  in_ = std::move(CreateBufferSlow("world"));
  result = HttpZeroCopyCheckRequest(conn_, in_, out_);
  ASSERT_EQ(kPacketFull, result);
  ASSERT_EQ(1, out_.size());
  ASSERT_EQ(0, in_.ByteSize());

  const auto& req = std::any_cast<const http::RequestPtr&>(out_.front());
  ASSERT_TRUE(req->IsPost());
  ASSERT_EQ("text/html", req->GetHeader("Content-Type"));
  ASSERT_TRUE(req->GetStream().AppendToRequest(10).OK());
  ASSERT_EQ(10, req->ContentLength());
  ASSERT_EQ("helloworld", FlattenSlow(req->GetNonContiguousBufferContent()));
}

TEST_F(HttpServerProtoCheckerTest, DefaultMultiNonChunkedFullPacket1) {
  // the headers and bodies of multiple packets are complete
  std::string s = "GET /form.html HTTP/1.1\r\nContent-Length:5\r\n\r\nhello";
  for (int i = 0; i < 3; ++i) {
    in_.Append(CreateBufferSlow(s.c_str(), s.size()));
  }

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);
  ASSERT_EQ(kPacketFull, result);
  ASSERT_EQ(3, out_.size());
  ASSERT_EQ(0, in_.ByteSize());
}

TEST_F(HttpServerProtoCheckerTest, DefaultMultiNonChunkedFullPacket2) {
  // the last packet's body is incomplete
  std::string s = "GET /form.html HTTP/1.1\r\nContent-Length:5\r\n\r\nhello";
  for (int i = 0; i < 2; ++i) {
    in_.Append(CreateBufferSlow(s.c_str(), s.size()));
  }
  in_.Append(CreateBufferSlow(s.c_str(), 10));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);
  ASSERT_EQ(kPacketFull, result);

  ASSERT_EQ(2, out_.size());
  ASSERT_EQ(10, in_.ByteSize());
}

TEST_F(HttpServerProtoCheckerTest, DefaultChunkedPacket1) {
  // chunked packets is complete
  std::string s =
      "POST /form.html HTTP/1.1\r\nTransfer-Encoding:chunked\r\n\r\n"
      "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
  in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

  ASSERT_EQ(kPacketFull, result);
  ASSERT_EQ(1, out_.size());
  ASSERT_EQ(0, in_.ByteSize());

  const auto& req = std::any_cast<const http::RequestPtr&>(out_.front());
  ASSERT_TRUE(req->IsPost());
  ASSERT_EQ("helloworld", FlattenSlow(req->GetNonContiguousBufferContent()));
}

TEST_F(HttpServerProtoCheckerTest, DefaultChunkedPacket2) {
  // chunked packets is incomplete
  std::string s = "POST /form.html HTTP/1.1\r\nTransfer-Encoding:chunked\r\n\r\n5\r\nhello\r\n";
  in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

  auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

  // In non-blocking mode, if the header is complete but the chunked data is incomplete, an intermediate request will be
  // placed in the connection until a subsequent check determines that the chunked data is complete. Once the chunked
  // data is complete, the request will be placed in the out queue.
  ASSERT_EQ(kPacketLess, result);
  ASSERT_EQ(0, out_.size());
  ASSERT_EQ(0, in_.ByteSize());

  // wait until the chunked data is complete
  in_ = std::move(CreateBufferSlow("5\r\nworld\r\n0\r\n\r\n"));
  result = HttpZeroCopyCheckRequest(conn_, in_, out_);
  ASSERT_EQ(kPacketFull, result);
  ASSERT_EQ(1, out_.size());
  ASSERT_EQ(0, in_.ByteSize());

  const auto& req = std::any_cast<const http::RequestPtr&>(out_.front());
  ASSERT_TRUE(req->IsPost());
  ASSERT_EQ("helloworld", FlattenSlow(req->GetNonContiguousBufferContent()));
}

TEST_F(HttpServerProtoCheckerTest, StreamNonChunkedPacket) {
  RunAsFiber([&] {
    // the header is complete, but the body is incomplete
    std::string s = "POST /form.html HTTP/1.1\r\nContent-Length:10\r\ncontent-type:text/html\r\n\r\nhello";
    in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

    auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

    // In blocking mode, if the header is complete but the body is incomplete, the incomplete request will still be
    // placed in the out queue. Subsequent data will be appended to this request until the packet is complete.
    ASSERT_EQ(kPacketFull, result);
    ASSERT_EQ(1, out_.size());
    ASSERT_EQ(0, in_.ByteSize());

    auto req = std::any_cast<http::RequestPtr>(out_.front());
    out_.pop_front();
    ASSERT_TRUE(req->IsPost());
    ASSERT_EQ("text/html", req->GetHeader("Content-Type"));
    ASSERT_EQ(10, req->ContentLength());
    ASSERT_EQ(5, req->GetStream().Size());

    // wait until the body is complete
    in_ = std::move(CreateBufferSlow("world"));
    result = HttpZeroCopyCheckRequest(conn_, in_, out_);
    ASSERT_EQ(kPacketLess, result);
    ASSERT_EQ(0, out_.size());
    ASSERT_EQ(0, in_.ByteSize());

    ASSERT_TRUE(req->GetStream().AppendToRequest(10).OK());
    ASSERT_EQ("helloworld", FlattenSlow(req->GetNonContiguousBufferContent()));
  });
}

TEST_F(HttpServerProtoCheckerTest, StreamChunkedPacket1) {
  // chunked packets is complete
  RunAsFiber([&] {
    std::string s =
        "POST /form.html HTTP/1.1\r\nTransfer-Encoding:chunked\r\n\r\n"
        "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
    NoncontiguousBufferBuilder builder{};
    builder.Append(s);
    in_ = builder.DestructiveGet();

    auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

    ASSERT_EQ(kPacketFull, result);
    ASSERT_EQ(1, out_.size());
    ASSERT_EQ(0, in_.ByteSize());

    const auto& req = std::any_cast<const http::RequestPtr&>(out_.front());
    ASSERT_TRUE(req->IsPost());
    auto found = req->GetHeader(http::kHeaderTransferEncoding);
    EXPECT_FALSE(found.empty());
    found = req->GetHeader(http::kHeaderContentLength);
    EXPECT_TRUE(found.empty());

    ASSERT_EQ(10, req->GetStream().Size());
    NoncontiguousBuffer out_buffer;
    ASSERT_TRUE(
        req->GetStream().Read(out_buffer, std::numeric_limits<size_t>::max(), std::chrono::milliseconds(1)).OK());
    ASSERT_EQ("helloworld", FlattenSlow(out_buffer));
  });
}

TEST_F(HttpServerProtoCheckerTest, StreamChunkedPacket2) {
  // chunked packets is incomplete
  RunAsFiber([&] {
    // chunked packets is incomplete
    std::string s = "POST /form.html HTTP/1.1\r\nTransfer-Encoding:chunked\r\n\r\n5\r\nhello\r\n";
    in_ = std::move(CreateBufferSlow(s.c_str(), s.size()));

    auto result = HttpZeroCopyCheckRequest(conn_, in_, out_);

    // In blocking mode, if the header is complete but the chunked data is incomplete, the incomplete request will still
    // be placed in the out queue. Subsequent chunked data will be appended to this request until the packet is
    // complete.
    ASSERT_EQ(kPacketFull, result);
    ASSERT_EQ(1, out_.size());
    ASSERT_EQ(0, in_.ByteSize());

    auto req = std::any_cast<http::RequestPtr>(out_.front());
    out_.pop_front();
    ASSERT_TRUE(req->IsPost());
    ASSERT_EQ(5, req->GetStream().Size());

    // wait until the chunked data is complete
    in_ = std::move(CreateBufferSlow("5\r\nworld\r\n0\r\n\r\n"));
    result = HttpZeroCopyCheckRequest(conn_, in_, out_);
    ASSERT_EQ(kPacketLess, result);
    ASSERT_EQ(0, out_.size());
    ASSERT_EQ(0, in_.ByteSize());

    ASSERT_EQ(10, req->GetStream().Size());
    NoncontiguousBuffer out_buffer;
    ASSERT_TRUE(
        req->GetStream().Read(out_buffer, std::numeric_limits<size_t>::max(), std::chrono::milliseconds(1)).OK());
    ASSERT_EQ("helloworld", FlattenSlow(out_buffer));
  });
}

}  // namespace trpc::testing
