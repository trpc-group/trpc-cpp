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

#include "trpc/stream/http/common/stream.h"

#include "gtest/gtest.h"

#include "trpc/stream/http/common/testing/mock_stream.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using RetCode = stream::RetCode;
using State = MockHttpCommonStream::State;
using Action = MockHttpCommonStream::Action;
using MockHttpCommonStreamPtr = RefPtr<MockHttpCommonStream>;

class HttpCommonStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    stream::StreamOptions options;
    options.stream_id = 1;
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;
    mock_stream_ = MakeRefCounted<MockHttpCommonStream>(std::move(options));
  }
  void TearDown() override {}

 protected:
  MockHttpCommonStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
};

TEST_F(HttpCommonStreamTest, Stop) {
  ASSERT_FALSE(mock_stream_->IsStateTerminate());
  EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1);
  mock_stream_->Stop();
  ASSERT_TRUE(mock_stream_->IsStateTerminate());
}

TEST_F(HttpCommonStreamTest, OnConnectionClosed) {
  ASSERT_FALSE(mock_stream_->IsStateTerminate());
  EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1);
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->OnConnectionClosed();
  ASSERT_TRUE(mock_stream_->IsStateTerminate());
}

TEST_F(HttpCommonStreamTest, HandleHeader) {
  // the read_state must be in the kInit state before handling the header
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kIdle);
  ASSERT_EQ(mock_stream_->HandleRecvMessage(MakeRefCounted<stream::HttpStreamHeader>()), RetCode::kError);

  // handle chunked headers
  EXPECT_CALL(*mock_stream_, OnHeader(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kInit);
  auto frame = MakeRefCounted<stream::HttpStreamHeader>();
  frame->GetMutableMetaData()->is_chunk = true;
  ASSERT_EQ(mock_stream_->HandleRecvMessage(frame), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kOpen);

  // handle headers with content length greater than 0
  EXPECT_CALL(*mock_stream_, OnHeader(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kInit);
  frame = MakeRefCounted<stream::HttpStreamHeader>();
  frame->GetMutableMetaData()->is_chunk = false;
  frame->GetMutableMetaData()->content_length = 1;
  ASSERT_EQ(mock_stream_->HandleRecvMessage(frame), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kOpen);

  // handle headers with content length equal to 0
  EXPECT_CALL(*mock_stream_, OnHeader(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kInit);
  frame = MakeRefCounted<stream::HttpStreamHeader>();
  frame->GetMutableMetaData()->is_chunk = false;
  frame->GetMutableMetaData()->content_length = 0;
  ASSERT_EQ(mock_stream_->HandleRecvMessage(frame), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kClosed);
}

TEST_F(HttpCommonStreamTest, HandleData) {
  // the read_state must be in the kOpen state before handling the data
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kIdle);
  ASSERT_EQ(mock_stream_->HandleRecvMessage(MakeRefCounted<stream::HttpStreamData>()), RetCode::kError);

  // handle the data normally
  EXPECT_CALL(*mock_stream_, OnData(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kOpen);
  ASSERT_EQ(mock_stream_->HandleRecvMessage(MakeRefCounted<stream::HttpStreamData>()), RetCode::kSuccess);
}

TEST_F(HttpCommonStreamTest, HandleEofAndTrailer) {
  // the read_state must be in the kOpen state before handling eof
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kIdle);
  ASSERT_EQ(mock_stream_->HandleRecvMessage(MakeRefCounted<stream::HttpStreamEof>()), RetCode::kError);

  // handle eof normally
  EXPECT_CALL(*mock_stream_, OnEof()).Times(1);
  mock_stream_->SetReadState(State::kOpen);
  ASSERT_EQ(mock_stream_->HandleRecvMessage(MakeRefCounted<stream::HttpStreamEof>()), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kClosed);

  // handle eof with trailer
  EXPECT_CALL(*mock_stream_, OnHeader(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kInit);
  auto frame = MakeRefCounted<stream::HttpStreamHeader>();
  frame->GetMutableMetaData()->has_trailer = true;
  frame->GetMutableMetaData()->is_chunk = true;
  ASSERT_EQ(mock_stream_->HandleRecvMessage(frame), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kOpen);
  EXPECT_CALL(*mock_stream_, OnEof()).Times(1);
  ASSERT_EQ(mock_stream_->HandleRecvMessage(MakeRefCounted<stream::HttpStreamEof>()), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kHalfClosed);
  // handle trailer
  EXPECT_CALL(*mock_stream_, OnTrailer(::testing::_)).Times(1);
  ASSERT_EQ(mock_stream_->HandleRecvMessage(MakeRefCounted<stream::HttpStreamTrailer>()), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kClosed);
}

TEST_F(HttpCommonStreamTest, HandleFullMessage) {
  EXPECT_CALL(*mock_stream_, OnHeader(::testing::_)).Times(1);
  EXPECT_CALL(*mock_stream_, OnData(::testing::_)).Times(1);
  EXPECT_CALL(*mock_stream_, OnEof()).Times(1);
  mock_stream_->SetReadState(State::kInit);
  auto full_frame = MakeRefCounted<stream::HttpStreamFullMessage>();
  auto header_frame = MakeRefCounted<stream::HttpStreamHeader>();
  header_frame->GetMutableMetaData()->is_chunk = true;
  full_frame->GetMutableFrames()->push_back(static_pointer_cast<stream::HttpStreamFrame>(header_frame));
  full_frame->GetMutableFrames()->push_back(
      static_pointer_cast<stream::HttpStreamFrame>(MakeRefCounted<stream::HttpStreamData>()));
  full_frame->GetMutableFrames()->push_back(
      static_pointer_cast<stream::HttpStreamFrame>(MakeRefCounted<stream::HttpStreamEof>()));

  ASSERT_EQ(mock_stream_->HandleRecvMessage(full_frame), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetReadState(), State::kClosed);
}

TEST_F(HttpCommonStreamTest, SendHeader) {
  // the write_state must be in the kInit state before sending header
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetWriteState(State::kIdle);
  auto frame = MakeRefCounted<stream::HttpStreamHeader>();
  NoncontiguousBuffer out;
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kError);
  ASSERT_TRUE(out.Empty());

  // send chunked headers
  mock_stream_->SetWriteState(State::kInit);
  frame = MakeRefCounted<stream::HttpStreamHeader>();
  frame->GetMutableHeader()->Add("Transfer-Encoding", "chunked");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kOpen);
  ASSERT_EQ(FlattenSlow(out), "Transfer-Encoding: chunked\r\n\r\n");

  // send headers with content length greater than 0
  mock_stream_->SetWriteState(State::kInit);
  frame = MakeRefCounted<stream::HttpStreamHeader>();
  frame->GetMutableHeader()->Add("Content-Length", "12");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kOpen);
  ASSERT_EQ(FlattenSlow(out), "Content-Length: 12\r\n\r\n");

  // send headers with content length equal to 0
  mock_stream_->SetWriteState(State::kInit);
  frame = MakeRefCounted<stream::HttpStreamHeader>();
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kClosed);
  ASSERT_EQ(FlattenSlow(out), "\r\n\r\n");

  // send headers with invalid content length
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetWriteState(State::kInit);
  frame = MakeRefCounted<stream::HttpStreamHeader>();
  frame->GetMutableHeader()->Add("Content-Length", "wyf");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kError);
  ASSERT_TRUE(out.Empty());
}

TEST_F(HttpCommonStreamTest, SendData) {
  // the write_state must be in the kOpen state before sending data
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetWriteState(State::kIdle);
  auto frame = MakeRefCounted<stream::HttpStreamData>();
  NoncontiguousBuffer out;
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kError);
  ASSERT_TRUE(out.Empty());

  // send chunked data
  mock_stream_->SetWriteState(State::kInit);
  auto header_frame = MakeRefCounted<stream::HttpStreamHeader>();
  header_frame->GetMutableHeader()->Add("Transfer-Encoding", "chunked");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header_frame, &out), RetCode::kSuccess);
  frame = MakeRefCounted<stream::HttpStreamData>();
  NoncontiguousBufferBuilder builder;
  builder.Append("test1");
  builder.Append("test2");
  *frame->GetMutableData() = builder.DestructiveGet();
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kSuccess);
  ASSERT_EQ(FlattenSlow(out), "a\r\ntest1test2\r\n");

  // send data with correct content length
  mock_stream_->SetWriteState(State::kInit);
  header_frame = MakeRefCounted<stream::HttpStreamHeader>();
  header_frame->GetMutableHeader()->Add("Content-Length", "10");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header_frame, &out), RetCode::kSuccess);
  frame = MakeRefCounted<stream::HttpStreamData>();
  builder = NoncontiguousBufferBuilder();
  builder.Append("test1");
  builder.Append("test2");
  *frame->GetMutableData() = builder.DestructiveGet();
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kSuccess);
  ASSERT_EQ(FlattenSlow(out), "test1test2");

  // send data with incorrect content length
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetWriteState(State::kInit);
  header_frame = MakeRefCounted<stream::HttpStreamHeader>();
  header_frame->GetMutableHeader()->Add("Content-Length", "10");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header_frame, &out), RetCode::kSuccess);
  frame = MakeRefCounted<stream::HttpStreamData>();
  builder = NoncontiguousBufferBuilder();
  builder.Append("test1");
  builder.Append("test22");
  *frame->GetMutableData() = builder.DestructiveGet();
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(frame, &out), RetCode::kError);
  ASSERT_TRUE(out.Empty());
}

TEST_F(HttpCommonStreamTest, TestSendEofAndTrailer) {
  NoncontiguousBuffer out;
  NoncontiguousBufferBuilder builder;
  RefPtr<stream::HttpStreamHeader> header = nullptr;
  RefPtr<stream::HttpStreamData> data = nullptr;
  RefPtr<stream::HttpStreamEof> eof = MakeRefCounted<stream::HttpStreamEof>();
  RefPtr<stream::HttpStreamTrailer> trailer = nullptr;

  // the write_state must be in the kOpen or kClose before sending header
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetWriteState(State::kIdle);
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(eof, &out), RetCode::kError);
  ASSERT_TRUE(out.Empty());

  // send eof with correct content length
  mock_stream_->SetWriteState(State::kInit);
  header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableHeader()->Add("Content-Length", "10");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header, &out), RetCode::kSuccess);
  data = MakeRefCounted<stream::HttpStreamData>();
  builder = NoncontiguousBufferBuilder();
  builder.Append("test1");
  builder.Append("test2");
  *data->GetMutableData() = builder.DestructiveGet();
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(data, &out), RetCode::kSuccess);
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(eof, &out), RetCode::kSuccess);
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kClosed);
  ASSERT_TRUE(out.Empty());

  // send eof with incorrect content length
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetWriteState(State::kInit);
  header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableHeader()->Add("Content-Length", "11");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header, &out), RetCode::kSuccess);
  data = MakeRefCounted<stream::HttpStreamData>();
  builder = NoncontiguousBufferBuilder();
  builder.Append("test1");
  builder.Append("test2");
  *data->GetMutableData() = builder.DestructiveGet();
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(data, &out), RetCode::kSuccess);
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(eof, &out), RetCode::kError);
  ASSERT_TRUE(out.Empty());

  // send eof in chunked mode
  mock_stream_->SetWriteState(State::kInit);
  header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableHeader()->Add("Transfer-Encoding", "chunked");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header, &out), RetCode::kSuccess);
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(eof, &out), RetCode::kSuccess);
  ASSERT_EQ(FlattenSlow(out), "0\r\n\r\n");
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kClosed);

  // send eof in chunked mode with trailer
  mock_stream_->SetWriteState(State::kInit);
  header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableHeader()->Add("Transfer-Encoding", "chunked");
  header->GetMutableHeader()->Add("Trailer", "my-trailer1");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header, &out), RetCode::kSuccess);
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(eof, &out), RetCode::kSuccess);
  ASSERT_EQ(FlattenSlow(out), "0\r\n");
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kHalfClosed);
  out = NoncontiguousBuffer();
  trailer = MakeRefCounted<stream::HttpStreamTrailer>();
  trailer->GetMutableHeader()->Add("my-trailer1", "my-trailer-value1");
  ASSERT_EQ(mock_stream_->HandleSendMessage(trailer, &out), RetCode::kSuccess);
  ASSERT_EQ(FlattenSlow(out), "my-trailer1: my-trailer-value1\r\n\r\n");

  // send eof in chunked mode with invalid trailer
  EXPECT_CALL(*mock_stream_, OnError(::testing::_)).Times(1);
  mock_stream_->SetWriteState(State::kInit);
  header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableHeader()->Add("Transfer-Encoding", "chunked");
  header->GetMutableHeader()->Add("Trailer", "my-trailer1");
  out = NoncontiguousBuffer();
  ASSERT_EQ(mock_stream_->HandleSendMessage(header, &out), RetCode::kSuccess);
  out = NoncontiguousBuffer();
  mock_stream_->HandleSendMessage(eof, &out);
  ASSERT_EQ(FlattenSlow(out), "0\r\n");
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kHalfClosed);
  out = NoncontiguousBuffer();
  trailer = MakeRefCounted<stream::HttpStreamTrailer>();
  trailer->GetMutableHeader()->Add("my-trailer", "my-trailer-value1");
  ASSERT_EQ(mock_stream_->HandleSendMessage(trailer, &out), RetCode::kError);
  ASSERT_TRUE(out.Empty());
}

}  // namespace trpc::testing
