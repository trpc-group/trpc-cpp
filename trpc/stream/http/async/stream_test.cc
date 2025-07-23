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

#include "trpc/stream/http/async/stream.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/stream/http/async/testing/mock_async_stream.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using State = MockHttpAsyncStream::State;
using MockHttpAsyncStreamPtr = RefPtr<MockHttpAsyncStream>;

class HttpAsyncStreamTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml");
    merge::StartRuntime();
  }

  static void TearDownTestCase() { merge::TerminateRuntime(); }

  void SetUp() override {
    stream::StreamOptions options;
    options.stream_id = 1;
    mock_stream_handler_ = CreateMockStreamHandler();
    options.stream_handler = mock_stream_handler_;
    mock_stream_ = MakeRefCounted<MockHttpAsyncStream>(std::move(options));
  }

  void TearDown() override {}

 protected:
  MockHttpAsyncStreamPtr mock_stream_{nullptr};
  MockStreamHandlerPtr mock_stream_handler_{nullptr};
};

using FnProm = std::shared_ptr<Promise<>>;
Future<> RunAsMerge(std::function<void(FnProm)> fn) {
  static auto thread_model = static_cast<MergeThreadModel*>(merge::RandomGetMergeThreadModel());
  auto prom = std::make_shared<Promise<>>();
  // always choose the same reactor
  thread_model->GetReactor(1)->SubmitTask([prom, fn]() { fn(prom); });
  return prom->GetFuture();
}

TEST_F(HttpAsyncStreamTest, AsyncReadHeader) {
  http::HttpHeader check_header;
  RefPtr<stream::HttpStreamHeader> header = nullptr;
  // read after the header arrives
  auto fut = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kInit);
    header = MakeRefCounted<stream::HttpStreamHeader>();
    header->GetMutableHeader()->Add("custom-key1", "custom-value1");
    header->GetMutableMetaData()->is_chunk = false;
    header->GetMutableMetaData()->content_length = 0;
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    mock_stream_->AsyncReadHeader().Then([&, prom](http::HttpHeader&& header) {
      check_header = header;
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(check_header.Get("custom-key1"), "custom-value1");

  // read first, then wait for the head to arrive
  auto fut1 = RunAsMerge([&](FnProm prom) {
    check_header = http::HttpHeader();
    mock_stream_->AsyncReadHeader().Then([&, prom](http::HttpHeader&& header) {
      check_header = header;
      prom->SetValue();
    });
    mock_stream_->SetReadState(State::kInit);
    header = MakeRefCounted<stream::HttpStreamHeader>();
    header->GetMutableHeader()->Add("custom-key1", "custom-value1");
    header->GetMutableMetaData()->is_chunk = false;
    header->GetMutableMetaData()->content_length = 0;
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
  });
  future::BlockingGet(std::move(fut1));
  ASSERT_EQ(check_header.Get("custom-key1"), "custom-value1");

  // read timeout
  int err_code = 0;
  auto fut2 = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kInit);
    mock_stream_->AsyncReadHeader(100).Then([&, prom](Future<http::HttpHeader>&& fut) {
      if (fut.IsFailed()) {
        err_code = fut.GetException().GetExceptionCode();
      }
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut2));

  header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableHeader()->Add("custom-key1", "custom-value1");
  header->GetMutableMetaData()->is_chunk = false;
  header->GetMutableMetaData()->content_length = 0;
  auto fut3 = RunAsMerge([&](FnProm prom) {
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    prom->SetValue();
  });

  future::BlockingGet(std::move(fut3));
  ASSERT_EQ(err_code, TRPC_STREAM_UNKNOWN_ERR);
}

NoncontiguousBuffer BuildBuffer(std::string data) {
  NoncontiguousBufferBuilder builder;
  builder.Append(data);
  return builder.DestructiveGet();
}

TEST_F(HttpAsyncStreamTest, AsyncReadChunk) {
  RefPtr<stream::HttpStreamHeader> header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableMetaData()->is_chunk = true;
  RefPtr<stream::HttpStreamData> data = MakeRefCounted<stream::HttpStreamData>();
  std::string check_data;
  // read after the data arrives
  auto fut1 = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kInit);
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    *data->GetMutableData() = BuildBuffer("hello chunk");
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    mock_stream_->AsyncReadChunk().Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut1));
  ASSERT_EQ(check_data, "hello chunk");

  // read before the data arrives
  auto fut2 = RunAsMerge([&](FnProm prom) {
    mock_stream_->AsyncReadChunk().Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
    mock_stream_->SetReadState(State::kInit);
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    *data->GetMutableData() = BuildBuffer("hello chunk2");
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
  });
  future::BlockingGet(std::move(fut2));
  ASSERT_EQ(check_data, "hello chunk2");

  // read timeout
  int err_code = 0;
  auto fut3 = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kInit);
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    mock_stream_->AsyncReadChunk(100).Then([&, prom](Future<NoncontiguousBuffer>&& fut) {
      if (fut.IsFailed()) {
        err_code = fut.GetException().GetExceptionCode();
      }
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut3));
  auto fut4 = RunAsMerge([&](FnProm prom) {
    *data->GetMutableData() = BuildBuffer("hello chunk2");
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut4));
  ASSERT_EQ(err_code, TRPC_STREAM_UNKNOWN_ERR);

  // use AsyncReadChunk interface in non-chunked mode
  auto fut5 = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kInit);
    header->GetMutableMetaData()->is_chunk = false;
    header->GetMutableMetaData()->content_length = 20;
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    *data->GetMutableData() = BuildBuffer("hello chunk");
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    mock_stream_->AsyncReadChunk().Then([&, prom](Future<NoncontiguousBuffer>&& fut) {
      if (fut.IsFailed()) {
        err_code = fut.GetException().GetExceptionCode();
      }
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut5));
  ASSERT_EQ(err_code, TRPC_STREAM_UNKNOWN_ERR);
}

void DoTestReadAtMost(RefPtr<stream::HttpStreamHeader> header, MockHttpAsyncStreamPtr mock_stream) {
  RefPtr<stream::HttpStreamData> data = MakeRefCounted<stream::HttpStreamData>();
  std::string check_data;
  // read all of the data
  auto fut1 = RunAsMerge([&](FnProm prom) {
    mock_stream->SetReadState(State::kInit);
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    *data->GetMutableData() = BuildBuffer("hello chunk");
    // there is no need to push eof here, because 10 bytes of data can be read
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    mock_stream->AsyncReadAtMost(std::numeric_limits<uint64_t>::max()).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut1));
  ASSERT_EQ(check_data, "hello chunk");
  // read 5 bytes
  auto fut2 = RunAsMerge([&](FnProm prom) {
    mock_stream->SetReadState(State::kInit);
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    *data->GetMutableData() = BuildBuffer("hello");
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    mock_stream->AsyncReadAtMost(10).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut2));
  ASSERT_EQ(check_data, "hello");
  // read 6 bytes
  auto fut3 = RunAsMerge([&](FnProm prom) {
    *data->GetMutableData() = BuildBuffer(" chunk");
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    mock_stream->AsyncReadAtMost(10).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut3));
  ASSERT_EQ(check_data, " chunk");
  // read util eof
  auto fut4 = RunAsMerge([&](FnProm prom) {
    mock_stream->PushRecvMessage(MakeRefCounted<stream::HttpStreamEof>());
    mock_stream->AsyncReadExactly(10).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut4));
  ASSERT_EQ(check_data, "");
}

TEST_F(HttpAsyncStreamTest, AsyncReadAtMostWithChunk) {
  // testing chunked mode
  RefPtr<stream::HttpStreamHeader> header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableMetaData()->is_chunk = true;
  DoTestReadAtMost(header, mock_stream_);
}

TEST_F(HttpAsyncStreamTest, AsyncReadAtMostWithContentLength) {
  // testing Content-Length mode
  RefPtr<stream::HttpStreamHeader> header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableMetaData()->is_chunk = false;
  header->GetMutableMetaData()->content_length = 11;
  DoTestReadAtMost(header, mock_stream_);
}

void DoTestReadExactly(RefPtr<stream::HttpStreamHeader> header, MockHttpAsyncStreamPtr mock_stream) {
  RefPtr<stream::HttpStreamData> data = MakeRefCounted<stream::HttpStreamData>();
  std::string check_data;
  // read all of the data
  auto fut1 = RunAsMerge([&](FnProm prom) {
    mock_stream->SetReadState(State::kInit);
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    *data->GetMutableData() = BuildBuffer("hello chunk");
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    mock_stream->PushRecvMessage(MakeRefCounted<stream::HttpStreamEof>());
    mock_stream->AsyncReadExactly(std::numeric_limits<uint64_t>::max()).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut1));
  ASSERT_EQ(check_data, "hello chunk");
  // read the first 10 bytes of data
  auto fut2 = RunAsMerge([&](FnProm prom) {
    mock_stream->SetReadState(State::kInit);
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
    *data->GetMutableData() = BuildBuffer("hello chunk");
    mock_stream->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(data));
    mock_stream->AsyncReadExactly(5).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut2));
  ASSERT_EQ(check_data, "hello");
  // read the last 6 bytes of data
  auto fut3 = RunAsMerge([&](FnProm prom) {
    mock_stream->AsyncReadExactly(6).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut3));
  ASSERT_EQ(check_data, " chunk");
  // read until eof
  auto fut4 = RunAsMerge([&](FnProm prom) {
    mock_stream->PushRecvMessage(MakeRefCounted<stream::HttpStreamEof>());
    mock_stream->AsyncReadExactly(6).Then([&, prom](NoncontiguousBuffer&& buf) {
      check_data = FlattenSlow(buf);
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut4));
  ASSERT_EQ(check_data, "");
}

TEST_F(HttpAsyncStreamTest, AsyncReadExactlyWithChunk) {
  // testing chunked mode
  RefPtr<stream::HttpStreamHeader> header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableMetaData()->is_chunk = true;
  DoTestReadExactly(header, mock_stream_);
}

TEST_F(HttpAsyncStreamTest, AsyncReadExactlyWithContentLength) {
  // testing Content-Length mode
  RefPtr<stream::HttpStreamHeader> header = MakeRefCounted<stream::HttpStreamHeader>();
  header->GetMutableMetaData()->is_chunk = false;
  header->GetMutableMetaData()->content_length = 11;
  DoTestReadExactly(header, mock_stream_);
}

TEST_F(HttpAsyncStreamTest, OnError) {
  // read all of the data
  EXPECT_CALL(*mock_stream_, Reset(::testing::_)).Times(1);
  int read_header_err = 0;
  Future<> fut_header = RunAsMerge([&](FnProm prom) {
    mock_stream_->AsyncReadHeader().Then(
        [&](Future<http::HttpHeader>&& fut) { read_header_err = fut.GetException().GetExceptionCode(); });
  });
  int read_data_err = 0;
  Future<> fut_data = RunAsMerge([&](FnProm prom) {
    mock_stream_->AsyncReadExactly(100).Then([&](Future<NoncontiguousBuffer>&& fut) {
      //
      read_data_err = fut.GetException().GetExceptionCode();
    });
  });
  RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kIdle);
    // trigger flow state transition error here
    mock_stream_->PushRecvMessage(MakeRefCounted<stream::HttpStreamHeader>());
  });
  future::BlockingGet(std::move(fut_header));
  future::BlockingGet(std::move(fut_data));
  ASSERT_EQ(read_header_err, TRPC_STREAM_UNKNOWN_ERR);
  ASSERT_EQ(read_data_err, TRPC_STREAM_UNKNOWN_ERR);
}

TEST_F(HttpAsyncStreamTest, PushSendMessage) {
  // send normally
  EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(0));
  auto fut1 = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetWriteState(State::kInit);
    mock_stream_->PushSendMessage(MakeRefCounted<stream::HttpStreamHeader>()).Then([&, prom]() {
      //
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut1));
  // send with exception
  EXPECT_CALL(*mock_stream_, Reset(::testing::_)).Times(1);
  int err_code = 0;
  auto fut2 = RunAsMerge([&](FnProm prom) {
    // Sends repeated header.
    mock_stream_->PushSendMessage(MakeRefCounted<stream::HttpStreamHeader>()).Then([&, prom](Future<>&& fut) {
      err_code = fut.GetException().GetExceptionCode();
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut2));
  ASSERT_EQ(err_code, TRPC_STREAM_UNKNOWN_ERR);
}

TEST_F(HttpAsyncStreamTest, PushSendMessageNetworkErr) {
  // network error
  EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));
  int err_code = 0;
  auto fut = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetWriteState(State::kInit);
    mock_stream_->PushSendMessage(MakeRefCounted<stream::HttpStreamHeader>()).Then([&, prom](Future<>&& fut) {
      err_code = fut.GetException().GetExceptionCode();
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(err_code, -1);
}

TEST_F(HttpAsyncStreamTest, PushSendMessageAtTerminate) {
  // Sending a message to let the stream close, the stream should be removed from the connection
  EXPECT_CALL(*mock_stream_handler_, SendMessage_rvr(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1);
  auto fut = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kClosed);
    mock_stream_->SetWriteState(State::kInit);
    auto header = MakeRefCounted<stream::HttpStreamHeader>();
    header->GetMutableHeader()->Add("Content-Length", "0");
    mock_stream_->PushSendMessage(static_pointer_cast<stream::HttpStreamFrame>(header));
  });
  future::BlockingGet(std::move(fut));
  ASSERT_TRUE(mock_stream_->IsStateTerminate());
}

}  // namespace trpc::testing
