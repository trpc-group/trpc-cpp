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

#include "trpc/stream/http/common/stream_handler.h"

#include "gtest/gtest.h"

#include "trpc/stream/http/common/stream.h"
#include "trpc/stream/http/common/testing/mock_stream.h"

namespace trpc::testing {

using namespace trpc::stream;

class HttpCommonStreamHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StreamOptions options;
    mock_stream_handler_ = MakeRefCounted<MockHttpCommonStreamHandler>(std::move(options));
  }
  void TearDown() override {}

  void ParseDequeOut(std::deque<std::any>& out) {
    header_ = static_pointer_cast<HttpStreamHeader>(std::any_cast<HttpStreamFramePtr>(out.front()));
    out.pop_front();
    if (out.empty()) {
      return;
    }
    do {
      auto frame = std::any_cast<HttpStreamFramePtr>(out.front());
      if (frame->GetFrameType() != HttpStreamFrame::HttpStreamFrameType::kData) {
        break;
      }
      data_.Append(*(static_pointer_cast<HttpStreamData>(frame)->GetMutableData()));
      out.pop_front();
    } while (!out.empty());
    if (out.empty()) {
      return;
    }
    eof_ = static_pointer_cast<HttpStreamEof>(std::any_cast<HttpStreamFramePtr>(out.front()));
    out.pop_front();
    if (out.empty()) {
      return;
    }
    trailer_ = static_pointer_cast<HttpStreamTrailer>(std::any_cast<HttpStreamFramePtr>(out.front()));
    out.pop_front();
  }

 protected:
  RefPtr<MockHttpCommonStreamHandler> mock_stream_handler_{nullptr};
  RefPtr<HttpStreamHeader> header_{nullptr};
  NoncontiguousBuffer data_;
  RefPtr<HttpStreamEof> eof_{nullptr};
  RefPtr<HttpStreamTrailer> trailer_{nullptr};
};

TEST_F(HttpCommonStreamHandlerTest, CreateStreamAndRemove) {
  // create stream
  ASSERT_TRUE(mock_stream_handler_->CreateStream(StreamOptions()) != nullptr);
  ASSERT_FALSE(mock_stream_handler_->IsNewStream(0, 0));

  // send msg
  bool is_send = false;
  mock_stream_handler_->GetMutableStreamOptions()->send = [&](IoMessage&& message) {
    is_send = true;
    return 0;
  };
  std::any placeholder = 1;
  mock_stream_handler_->SendMessage(placeholder, NoncontiguousBuffer{});
  ASSERT_TRUE(is_send);

  // remove stream
  ASSERT_EQ(mock_stream_handler_->RemoveStream(0), 0);
  ASSERT_TRUE(mock_stream_handler_->IsNewStream(0, 0));
}

NoncontiguousBuffer BuildBuffer(std::string buf) {
  NoncontiguousBufferBuilder builder;
  builder.Append(buf);
  return builder.DestructiveGet();
}

TEST_F(HttpCommonStreamHandlerTest, ParseContentLength0Ok) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("Content-Length: 0\r\n\r\n");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 1);
  ParseDequeOut(out);
  ASSERT_TRUE(out.empty());
  ASSERT_EQ(header_->GetMutableHeader()->Get("Content-Length"), "0");
  ASSERT_EQ(header_->GetMutableMetaData()->content_length, 0);
  ASSERT_EQ(header_->GetMutableMetaData()->is_chunk, false);
  ASSERT_EQ(header_->GetMutableMetaData()->has_trailer, false);
  ASSERT_TRUE(data_.Empty());
  ASSERT_TRUE(eof_ == nullptr);
  ASSERT_TRUE(trailer_ == nullptr);
}

TEST_F(HttpCommonStreamHandlerTest, ParseContentLength12Ok) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("Content-Length: 12\r\n\r\nhello stream");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 3);
  ParseDequeOut(out);
  ASSERT_TRUE(out.empty());
  ASSERT_EQ(header_->GetMutableHeader()->Get("Content-Length"), "12");
  ASSERT_EQ(header_->GetMutableMetaData()->content_length, 12);
  ASSERT_EQ(header_->GetMutableMetaData()->is_chunk, false);
  ASSERT_EQ(header_->GetMutableMetaData()->has_trailer, false);
  ASSERT_EQ(FlattenSlow(data_), "hello stream");
  ASSERT_TRUE(eof_ != nullptr);
  ASSERT_TRUE(trailer_ == nullptr);
}

TEST_F(HttpCommonStreamHandlerTest, TestParseHttpChunkWithDataOk) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("Transfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n7\r\n stream\r\n0\r\n\r\n");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 4);
  ParseDequeOut(out);
  ASSERT_TRUE(out.empty());
  ASSERT_EQ(header_->GetMutableHeader()->Get("Transfer-Encoding"), "chunked");
  ASSERT_EQ(header_->GetMutableMetaData()->is_chunk, true);
  ASSERT_EQ(header_->GetMutableMetaData()->has_trailer, false);
  ASSERT_EQ(FlattenSlow(data_), "hello stream");
  ASSERT_TRUE(eof_ != nullptr);
  ASSERT_TRUE(trailer_ == nullptr);
}

TEST_F(HttpCommonStreamHandlerTest, ParseHttpChunkWithNoDataOk) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("Transfer-Encoding: chunked\r\n\r\n0\r\n\r\n");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 2);
  ParseDequeOut(out);
  ASSERT_TRUE(out.empty());
  ASSERT_EQ(header_->GetMutableHeader()->Get("Transfer-Encoding"), "chunked");
  ASSERT_EQ(header_->GetMutableMetaData()->is_chunk, true);
  ASSERT_EQ(header_->GetMutableMetaData()->has_trailer, false);
  ASSERT_EQ(FlattenSlow(data_), "");
  ASSERT_TRUE(eof_ != nullptr);
  ASSERT_TRUE(trailer_ == nullptr);
}

TEST_F(HttpCommonStreamHandlerTest, ParseHttpChunkWithTrailerOk) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf =
      BuildBuffer("Transfer-Encoding: chunked\r\nTrailer: t1\r\n\r\n5\r\nhello\r\n7\r\n stream\r\n0\r\nt1: v1\r\n\r\n");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 5);
  ParseDequeOut(out);
  ASSERT_TRUE(out.empty());
  ASSERT_EQ(header_->GetMutableHeader()->Get("Transfer-Encoding"), "chunked");
  ASSERT_EQ(header_->GetMutableMetaData()->is_chunk, true);
  ASSERT_EQ(header_->GetMutableMetaData()->has_trailer, true);
  ASSERT_EQ(FlattenSlow(data_), "hello stream");
  ASSERT_TRUE(eof_ != nullptr);
  ASSERT_EQ(trailer_->GetMutableHeader()->Get("t1"), "v1");
}

TEST_F(HttpCommonStreamHandlerTest, ParseHttpHeaderInCompleteAndError) {
  std::deque<std::any> out;

  NoncontiguousBuffer buf = BuildBuffer("Content-Length: 1");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), -2);
  ASSERT_EQ(out.size(), 0);

  buf = BuildBuffer("Content\r-Le\nngth: 1");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), -1);
  ASSERT_EQ(out.size(), 0);
}

TEST_F(HttpCommonStreamHandlerTest, ParseHttpChunkDataInCompleteAndError) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("Transfer-Encoding: chunked\r\n\r\n");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 1);
  out.pop_front();
  ASSERT_TRUE(out.empty());

  buf = BuildBuffer("5\r\nhe");
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), -2);
  ASSERT_EQ(out.size(), 0);

  buf = BuildBuffer("zz\rh");
  ASSERT_EQ(out.size(), 0);
  ASSERT_EQ(mock_stream_handler_->ParseMessage(&buf, &out), -1);
}

}  // namespace trpc::testing
