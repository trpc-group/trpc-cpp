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

#include "trpc/stream/http/async/stream_reader_writer.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using namespace stream;

class MockAsyncStreamHandler : public MockStreamHandler {
 public:
  int SendMessage(const std::any& data, NoncontiguousBuffer&& buff) override {
    data_.Append(buff);
    return 0;
  }
  NoncontiguousBuffer GetSendMessage() { return data_; }

 private:
  NoncontiguousBuffer data_;
};
using MockAsyncStreamHandlerPtr = RefPtr<MockAsyncStreamHandler>;

class HttpAsyncStreamReaderWriterTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml");
    merge::StartRuntime();
  }

  static void TearDownTestCase() { merge::TerminateRuntime(); }

  void SetUp() override {
    StreamOptions server_options;
    auto svr_ctx = MakeRefCounted<ServerContext>();
    server_options.context.context = svr_ctx;
    server_stream_mock_handler_ = MakeRefCounted<MockAsyncStreamHandler>();
    server_options.stream_handler = server_stream_mock_handler_;
    server_stream_ = MakeRefCounted<HttpServerAsyncStream>(std::move(server_options));
    server_rw_ = MakeRefCounted<HttpServerAsyncStreamReaderWriter>(server_stream_);

    StreamOptions client_options;
    client_stream_mock_handler_ = MakeRefCounted<MockAsyncStreamHandler>();
    client_options.stream_handler = client_stream_mock_handler_;
    client_stream_ = MakeRefCounted<HttpClientAsyncStream>(std::move(client_options));
    client_rw_ = MakeRefCounted<HttpClientAsyncStreamReaderWriter>(client_stream_);
  }

  void TearDown() override {}

 protected:
  HttpServerAsyncStreamPtr server_stream_{nullptr};
  HttpServerAsyncStreamReaderWriterPtr server_rw_{nullptr};
  MockAsyncStreamHandlerPtr server_stream_mock_handler_{nullptr};

  HttpClientAsyncStreamPtr client_stream_{nullptr};
  HttpClientAsyncStreamReaderWriterPtr client_rw_{nullptr};
  MockAsyncStreamHandlerPtr client_stream_mock_handler_{nullptr};
};

using FnProm = std::shared_ptr<Promise<>>;
Future<> RunAsMerge(std::function<void(FnProm)> fn) {
  static auto thread_model = static_cast<MergeThreadModel*>(merge::RandomGetMergeThreadModel());
  auto prom = std::make_shared<Promise<>>();
  // always choose the same reactor
  thread_model->GetReactor(1)->SubmitTask([prom, fn]() { fn(prom); });
  return prom->GetFuture();
}

// The reader and writer mainly restrict the exposed interfaces of the stream, but the interface testing of the stream
// has already been implemented in the stream's unit tests. Therefore, here we only test the provided utility methods.


TEST_F(HttpAsyncStreamReaderWriterTest, WriteFullRequest) {
  auto fut = RunAsMerge([&](FnProm prom) {
    http::HttpRequest req;
    req.SetUrl("/hello");
    req.SetMethod("POST");
    req.SetVersion("1.0");
    req.SetHeader("Content-Length", "23");
    req.SetHeader("req-custom-header1", "req-custom-value1");
    req.SetContent("hello world from client");
    WriteFullRequest(client_rw_, std::move(req)).Then([prom]() {
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(FlattenSlow(client_stream_mock_handler_->GetSendMessage()),
            "POST /hello HTTP/1.0\r\nContent-Length: 23\r\nreq-custom-header1: req-custom-value1\r\n\r\nhello world "
            "from client");
}

TEST_F(HttpAsyncStreamReaderWriterTest, WriteFullResponse) {
  auto fut = RunAsMerge([&](FnProm prom) {
    http::HttpResponse rsp;
    rsp.SetStatus(202);
    rsp.SetReasonPhrase("OK2");
    rsp.SetVersion("1.0");
    rsp.SetHeader("Content-Length", "23");
    rsp.SetHeader("rsp-custom-header1", "rsp-custom-value1");
    rsp.SetContent("hello world from server");
    WriteFullResponse(server_rw_, std::move(rsp)).Then([prom]() { prom->SetValue(); });
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(FlattenSlow(server_stream_mock_handler_->GetSendMessage()),
            "HTTP/1.0 202 OK2\r\nContent-Length: 23\r\nrsp-custom-header1: rsp-custom-value1\r\n\r\nhello world "
            "from server");
}

TEST_F(HttpAsyncStreamReaderWriterTest, ReadFullRequest) {
  http::HttpRequestPtr check_req = nullptr;
  bool init_inovke = false;
  server_stream_->GetMutableStreamOptions()->callbacks.handle_streaming_rpc_cb = [&](STransportReqMsg* recv) {
    init_inovke = true;
  };
  auto fut = RunAsMerge([&](FnProm prom) {
    auto line = MakeRefCounted<HttpStreamRequestLine>();
    line->GetMutableHttpRequestLine()->method = "POST";
    line->GetMutableHttpRequestLine()->request_uri = "/hello";
    line->GetMutableHttpRequestLine()->version = "1.0";
    server_stream_->PushRecvMessage(line);
    server_stream_->GetMutableParameters()->Set("foo", "bar");
    ReadFullRequest(server_rw_).Then([&, prom](http::HttpRequestPtr req) {
      check_req = req;
      prom->SetValue();
    });
    auto header = MakeRefCounted<HttpStreamHeader>();
    header->GetMutableHeader()->Add("Content-Length", "23");
    header->GetMutableHeader()->Add("req-custom-header1", "req-custom-value1");
    header->GetMutableMetaData()->is_chunk = false;
    header->GetMutableMetaData()->content_length = 23;
    server_stream_->PushRecvMessage(header);
    auto data = MakeRefCounted<HttpStreamData>();
    NoncontiguousBufferBuilder buidler;
    buidler.Append("hello world from client");
    *data->GetMutableData() = buidler.DestructiveGet();
    server_stream_->PushRecvMessage(data);
  });
  future::BlockingGet(std::move(fut));
  ASSERT_TRUE(init_inovke);
  ASSERT_EQ(check_req->GetMethod(), "POST");
  ASSERT_EQ(check_req->GetVersion(), "1.0");
  ASSERT_EQ(check_req->GetUrl(), "/hello");
  ASSERT_EQ(server_stream_->GetParameters().Path("foo"), "bar");
  ASSERT_EQ(check_req->GetParameters().Path("foo"), "bar");
  ASSERT_EQ(check_req->GetHeader("req-custom-header1"), "req-custom-value1");
  ASSERT_EQ(check_req->GetContent(), "hello world from client");
}

TEST_F(HttpAsyncStreamReaderWriterTest, ReadFullResponse) {
  http::HttpResponsePtr check_rsp = nullptr;
  auto fut = RunAsMerge([&](FnProm prom) {
    ReadFullResponse(client_rw_).Then([&, prom](http::HttpResponsePtr rsp) {
      check_rsp = rsp;
      prom->SetValue();
    });
    auto line = MakeRefCounted<HttpStreamStatusLine>();
    line->GetMutableHttpStatusLine()->status_code = 202;
    line->GetMutableHttpStatusLine()->status_text = "OK2";
    line->GetMutableHttpStatusLine()->version = "1.0";
    client_stream_->PushRecvMessage(line);
    auto header = MakeRefCounted<HttpStreamHeader>();
    header->GetMutableHeader()->Add("Content-Length", "23");
    header->GetMutableHeader()->Add("rsp-custom-header1", "rsp-custom-value1");
    header->GetMutableMetaData()->is_chunk = false;
    header->GetMutableMetaData()->content_length = 23;
    client_stream_->PushRecvMessage(header);
    auto data = MakeRefCounted<HttpStreamData>();
    NoncontiguousBufferBuilder buidler;
    buidler.Append("hello world from server");
    *data->GetMutableData() = buidler.DestructiveGet();
    client_stream_->PushRecvMessage(data);
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(check_rsp->GetStatus(), 202);
  ASSERT_EQ(check_rsp->GetReasonPhrase(), "OK2");
  ASSERT_EQ(check_rsp->GetVersion(), "1.0");
  ASSERT_EQ(check_rsp->GetHeader("rsp-custom-header1"), "rsp-custom-value1");
  ASSERT_EQ(check_rsp->GetContent(), "hello world from server");
}

}  // namespace trpc::testing
