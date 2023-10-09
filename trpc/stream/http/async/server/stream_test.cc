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

#include "trpc/stream/http/async/server/stream.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/stream/http/async/server/testing/mock_server_async_stream.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using namespace trpc::stream;

using State = MockHttpServerAsyncStream::State;
using MockHttpServerAsyncStreamPtr = RefPtr<MockHttpServerAsyncStream>;

class TestMockStreamHandler : public MockStreamHandler {
 public:
  int SendMessage(const std::any& data, NoncontiguousBuffer&& buff) override {
    send_buf.Append(buff);
    return 0;
  }

 public:
  NoncontiguousBuffer send_buf;
};

class HttpServerAsyncStreamTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml");
    merge::StartRuntime();
  }

  static void TearDownTestCase() { merge::TerminateRuntime(); }

  void SetUp() override {
    auto svr_ctx = MakeRefCounted<ServerContext>();
    StreamOptions options;
    options.stream_id = 1;
    options.fiber_mode = true;
    mock_stream_handler_ = MakeRefCounted<TestMockStreamHandler>();
    options.stream_handler = mock_stream_handler_;
    options.context.context = svr_ctx;
    options.callbacks.handle_streaming_rpc_cb = [&](STransportReqMsg* recv) {};
    mock_stream_ = MakeRefCounted<MockHttpServerAsyncStream>(std::move(options));
  }

  void TearDown() override {}

 protected:
  MockHttpServerAsyncStreamPtr mock_stream_{nullptr};
  RefPtr<TestMockStreamHandler> mock_stream_handler_{nullptr};
  bool init_invoked_{false};
};

TEST_F(HttpServerAsyncStreamTest, ErrorCode) {
  ASSERT_EQ(mock_stream_->GetReadTimeoutErrorCode(), TrpcRetCode::TRPC_STREAM_SERVER_READ_TIMEOUT_ERR);
  ASSERT_EQ(mock_stream_->GetWriteTimeoutErrorCode(), TrpcRetCode::TRPC_STREAM_SERVER_WRITE_TIMEOUT_ERR);
  ASSERT_EQ(mock_stream_->GetNetWorkErrorCode(), TrpcRetCode::TRPC_STREAM_SERVER_NETWORK_ERR);
}

using FnProm = std::shared_ptr<Promise<>>;
Future<> RunAsMerge(std::function<void(FnProm)> fn) {
  static auto thread_model = static_cast<MergeThreadModel*>(merge::RandomGetMergeThreadModel());
  auto prom = std::make_shared<Promise<>>();
  // always choose the same reactor
  thread_model->GetReactor(1)->SubmitTask([prom, fn]() { fn(prom); });
  return prom->GetFuture();
}

TEST_F(HttpServerAsyncStreamTest, RecvStartLine) {
  bool init_invoked = false;
  mock_stream_->GetMutableStreamOptions()->callbacks.handle_streaming_rpc_cb = [&](STransportReqMsg* recv) {
    init_invoked = true;
  };
  auto fut = RunAsMerge([&](FnProm prom) {
    auto start_line = MakeRefCounted<HttpStreamRequestLine>();
    start_line->GetMutableHttpRequestLine()->method = "POST";
    start_line->GetMutableHttpRequestLine()->request_uri = "/hello";
    start_line->GetMutableHttpRequestLine()->version = "1.1";
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(start_line));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  const HttpRequestLine& start_line = mock_stream_->GetRequestLine();
  ASSERT_EQ(start_line.method, "POST");
  ASSERT_EQ(start_line.request_uri, "/hello");
  ASSERT_EQ(start_line.version, "1.1");
  ASSERT_EQ(mock_stream_->GetReadState(), State::kInit);
  ASSERT_TRUE(init_invoked);
}

TEST_F(HttpServerAsyncStreamTest, GetFullRequest) {
  auto fut = RunAsMerge([&](FnProm prom) {
    std::deque<std::any> frames;
    // start line
    auto start_line = MakeRefCounted<HttpStreamRequestLine>();
    start_line->GetMutableHttpRequestLine()->method = "POST";
    start_line->GetMutableHttpRequestLine()->request_uri = "/hello";
    start_line->GetMutableHttpRequestLine()->version = "1.1";
    frames.push_back(static_pointer_cast<HttpStreamFrame>(start_line));
    // header
    auto header = MakeRefCounted<HttpStreamHeader>();
    header->GetMutableMetaData()->is_chunk = false;
    header->GetMutableMetaData()->content_length = 12;
    header->GetMutableHeader()->Add("Content-Length", "12");
    header->GetMutableHeader()->Add("custom-header1", "custom-value1");
    frames.push_back(static_pointer_cast<HttpStreamFrame>(header));
    // body
    auto data = MakeRefCounted<HttpStreamData>();
    NoncontiguousBufferBuilder builder;
    builder.Append("hello stream");
    *data->GetMutableData() = builder.DestructiveGet();
    frames.push_back(static_pointer_cast<HttpStreamFrame>(data));
    // full message
    auto full_msg = MakeRefCounted<HttpStreamFullMessage>();
    *full_msg->GetMutableFrames() = std::move(frames);
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(full_msg));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  http::HttpRequestPtr req = mock_stream_->GetFullRequest();
  ASSERT_EQ(req->GetUrl(), "/hello");
  ASSERT_EQ(req->GetVersion(), "1.1");
  ASSERT_EQ(req->GetMethod(), "POST");
  ASSERT_EQ(req->GetHeader("Content-Length"), "12");
  ASSERT_EQ(req->GetHeader("custom-header1"), "custom-value1");
  ASSERT_EQ(req->GetContent(), "hello stream");
}

TEST_F(HttpServerAsyncStreamTest, SendStartLine) {
  NoncontiguousBuffer buf;
  auto fut = RunAsMerge([&](FnProm prom) {
    auto start_line = MakeRefCounted<HttpStreamStatusLine>();
    start_line->GetMutableHttpStatusLine()->status_code = 202;
    start_line->GetMutableHttpStatusLine()->status_text = "OK2";
    start_line->GetMutableHttpStatusLine()->version = "1.1";
    mock_stream_->PushSendMessage(static_pointer_cast<stream::HttpStreamFrame>(start_line)).Then([&, prom]() {
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(FlattenSlow(mock_stream_handler_->send_buf), "HTTP/1.1 202 OK2\r\n");
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kInit);
}

TEST_F(HttpServerAsyncStreamTest, OnErrorAndReset) {
  // It will trigger an error because the read_state is not correct.
  EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1);
  auto fut = RunAsMerge([&](FnProm prom) {
    mock_stream_->PushRecvMessage(static_pointer_cast<stream::HttpStreamFrame>(MakeRefCounted<HttpStreamHeader>()));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(mock_stream_->GetStatus().GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
}

}  // namespace trpc::testing
