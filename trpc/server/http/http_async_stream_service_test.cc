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

#include "trpc/server/http/http_async_stream_service.h"

#include <deque>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/testing/server_filter_testing.h"
#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/server/service_adapter.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/transport/server/testing/server_transport_testing.h"
#include "trpc/util/string_helper.h"

namespace trpc::testing {

using namespace stream;

class MockAsyncServerTransport : public TestServerTransport {
 public:
  int SendMsg(trpc::STransportRspMsg* msg) override {
    send_queue_.push_back(msg);
    return 0;
  }

  std::deque<trpc::STransportRspMsg*>& GetSendQueue() { return send_queue_; }

 private:
  std::deque<trpc::STransportRspMsg*> send_queue_;
};

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

class HttpAsyncStreamServiceTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml");
    merge::StartRuntime();
    codec::Init();
  }

  static void TearDownTestCase() {
    codec::Destroy();
    merge::TerminateRuntime();
  }

  void SetUp() override {
    service_ = std::make_shared<HttpAsyncStreamService>();
    service_->RegisterUnaryFuncHandler(
        trpc::http::OperationType::POST, trpc::http::Path("/hello"),
        std::make_shared<trpc::http::FuncHandler>(
            [](trpc::ServerContextPtr context, trpc::http::HttpRequestPtr req, trpc::http::HttpReply* rsp) {
              rsp->SetContent(req->GetContent());
              return trpc::kDefaultStatus;
            },
            "json"));
    service_->RegisterUnaryFuncHandler(
        trpc::http::OperationType::POST, trpc::http::Path("/hello-timeout"),
        std::make_shared<trpc::http::FuncHandler>(
            [](trpc::ServerContextPtr context, trpc::http::HttpRequestPtr req, trpc::http::HttpReply* rsp) {
              context->SetRecvTimestampUs(0);
              return trpc::kDefaultStatus;
            },
            "json"));
    service_->RegisterAsyncStreamMethod(
        trpc::http::OperationType::POST, trpc::http::Path("/hello-stream"),
        [](const ServerContextPtr& ctx, HttpServerAsyncStreamReaderWriterPtr rw) -> Future<> {
          return ReadFullRequest(rw, 1000).Then([rw](http::HttpRequestPtr&& req) {
            http::HttpResponse rsp;
            rsp.SetContent(req->GetContent());
            return WriteFullResponse(rw, std::move(rsp));
          });
        });

    mock_filter_ = std::make_shared<MockServerFilter>();
    std::vector<FilterPoint> points = {FilterPoint::SERVER_POST_RECV_MSG, FilterPoint::SERVER_PRE_RPC_INVOKE,
                                       FilterPoint::SERVER_POST_RPC_INVOKE, FilterPoint::SERVER_PRE_SEND_MSG};
    EXPECT_CALL(*mock_filter_, Name()).WillRepeatedly(::testing::Return("test_filters"));
    EXPECT_CALL(*mock_filter_, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
    service_->GetFilterController().AddMessageServerFilter(mock_filter_);
    transport_ = std::make_unique<MockAsyncServerTransport>();
    service_->SetServerTransport(transport_.get());
    ServiceAdapterOption option;
    option.queue_timeout = 1000;
    option.idle_time = 60000;
    option.protocol = "http";
    adapter_ = std::make_unique<trpc::ServiceAdapter>(std::move(option));
    service_->SetAdapter(adapter_.get());

    http::RequestPtr request = std::make_shared<http::Request>(1000, false);
    svr_ctx_ = MakeTestServerContext("http", service_.get(), std::move(request));

    StreamOptions stream_options;
    mock_stream_handler_ = MakeRefCounted<MockAsyncStreamHandler>();
    stream_options.stream_handler = mock_stream_handler_;
    stream_options.context.context = svr_ctx_;
    stream_options.callbacks.handle_streaming_rpc_cb = [&](STransportReqMsg* recv) {
      recv->context->SetFilterController(&(service_->GetFilterController()));
      service_->HandleTransportStreamMessage(recv);
    };
    server_stream_ = MakeRefCounted<HttpServerAsyncStream>(std::move(stream_options));

    svr_ctx_->SetStreamReaderWriterProvider(server_stream_);
  }

  void TearDown() override {}

  void SetRecvSendFilterMockExpect() {
    EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::SERVER_POST_RECV_MSG, ::testing::_)).Times(1);
    EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::SERVER_PRE_SEND_MSG, ::testing::_)).Times(1);
  }

  void SetRpcFilterMockExpect() {
    EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::SERVER_PRE_RPC_INVOKE, ::testing::_)).Times(1);
    EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::SERVER_POST_RPC_INVOKE, ::testing::_)).Times(1);
  }

  void SetStreamHandlerMockExpect() { EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1); }

 protected:
  HttpServerAsyncStreamPtr server_stream_{nullptr};
  RefPtr<MockAsyncStreamHandler> mock_stream_handler_{nullptr};
  std::shared_ptr<HttpAsyncStreamService> service_{nullptr};
  std::shared_ptr<MockServerFilter> mock_filter_{nullptr};
  ServerContextPtr svr_ctx_{nullptr};
  std::unique_ptr<MockAsyncServerTransport> transport_;
  std::unique_ptr<trpc::ServiceAdapter> adapter_;
};

using FnProm = std::shared_ptr<Promise<>>;
Future<> RunAsMerge(std::function<void(FnProm)> fn) {
  static auto thread_model = static_cast<MergeThreadModel*>(merge::RandomGetMergeThreadModel());
  auto prom = std::make_shared<Promise<>>();
  // always choose the same reactor
  thread_model->GetReactor(1)->SubmitTask([prom, fn]() { fn(prom); });
  return prom->GetFuture();
}

RefPtr<HttpStreamRequestLine> ConstructRequestLine(std::string method, std::string path) {
  auto line = MakeRefCounted<HttpStreamRequestLine>();
  line->GetMutableHttpRequestLine()->method = method;
  line->GetMutableHttpRequestLine()->request_uri = path;
  return line;
}

RefPtr<HttpStreamStatusLine> ConstructResponseStartLine(int status_code, std::string status_text) {
  auto line = MakeRefCounted<HttpStreamStatusLine>();
  line->GetMutableHttpStatusLine()->status_code = status_code;
  line->GetMutableHttpStatusLine()->status_text = status_text;
  return line;
}

RefPtr<HttpStreamHeader> ConstructHeader(std::unordered_map<std::string, std::string> headers) {
  auto header = MakeRefCounted<HttpStreamHeader>();
  for (const auto& it : headers) {
    if (it.first == "Content-Length") {
      header->GetMutableMetaData()->is_chunk = false;
      header->GetMutableMetaData()->content_length = TryParse<size_t>(it.second).value();
    } else if (it.first == "Transfer-Encoding" && it.second == "chunked") {
      header->GetMutableMetaData()->is_chunk = true;
    }
    header->GetMutableHeader()->Add(it.first, it.second);
  }
  return header;
}

RefPtr<HttpStreamData> ConstructData(std::string content) {
  auto data = MakeRefCounted<HttpStreamData>();
  NoncontiguousBufferBuilder builder;
  builder.Append(content);
  *data->GetMutableData() = builder.DestructiveGet();
  return data;
}

RefPtr<HttpStreamFullMessage> ConstructRequestFullMessage(std::string method, std::string path,
                                                          std::unordered_map<std::string, std::string> headers,
                                                          std::string content) {
  std::deque<std::any> frames;
  frames.push_back(static_pointer_cast<HttpStreamFrame>(ConstructRequestLine(method, path)));
  frames.push_back(static_pointer_cast<HttpStreamFrame>(ConstructHeader(headers)));
  frames.push_back(static_pointer_cast<HttpStreamFrame>(ConstructData(content)));
  auto full_msg = MakeRefCounted<HttpStreamFullMessage>();
  *full_msg->GetMutableFrames() = std::move(frames);
  return full_msg;
}

std::string FetchAllDataFromAdapterSendQueue(MockAsyncServerTransport* transport) {
  std::deque<STransportRspMsg*>& send_queue = transport->GetSendQueue();
  std::string content;
  while (!send_queue.empty()) {
    auto it = send_queue.front();
    content += FlattenSlow(it->buffer);
    send_queue.pop_front();
  }
  return content;
}

// We trigger streaming distribution processing through the PushRecvMessage interface, and then intercept the contents
// of the response packet to confirm whether the processing flow is correct.

TEST_F(HttpAsyncStreamServiceTest, HandleUnaryMethodFullMessageOk) {
  SetRecvSendFilterMockExpect();
  SetRpcFilterMockExpect();
  SetStreamHandlerMockExpect();
  // push a full message
  server_stream_->PushRecvMessage(
      ConstructRequestFullMessage("POST", "/hello", {{"Content-Length", "14"}}, "hello full msg"));
  std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());

  ASSERT_TRUE(send_data.find("HTTP/1.1 200 OK\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("Content-Length: 14\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("hello full msg") != std::string::npos);
}

TEST_F(HttpAsyncStreamServiceTest, RequestTimeoutBeforeRpcInvoke) {
  SetRecvSendFilterMockExpect();
  SetStreamHandlerMockExpect();
  svr_ctx_->SetRecvTimestampUs(0);
  server_stream_->PushRecvMessage(
      ConstructRequestFullMessage("POST", "/hello", {{"Content-Length", "14"}}, "hello full msg"));
  std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());

  ASSERT_TRUE(send_data.find("HTTP/1.1 504 Gateway Timeout\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("Content-Length: 15\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("gateway timeout") != std::string::npos);
}

TEST_F(HttpAsyncStreamServiceTest, MethodNotFound) {
  SetRecvSendFilterMockExpect();
  SetStreamHandlerMockExpect();
  server_stream_->PushRecvMessage(
      ConstructRequestFullMessage("POST", "/not-exist-url", {{"Content-Length", "14"}}, "hello full msg"));
  std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());

  ASSERT_TRUE(send_data.find("HTTP/1.1 404 Not Found\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("Content-Length: 9\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("not found") != std::string::npos);
}

TEST_F(HttpAsyncStreamServiceTest, RequestTimeoutAfterRpcInvokeFullMessage) {
  SetRecvSendFilterMockExpect();
  SetRpcFilterMockExpect();
  SetStreamHandlerMockExpect();
  server_stream_->PushRecvMessage(
      ConstructRequestFullMessage("POST", "/hello-timeout", {{"Content-Length", "14"}}, "hello full msg"));
  std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());

  ASSERT_TRUE(send_data.find("HTTP/1.1 408 Request Timeout\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("Content-Length: 22\r\n") != std::string::npos);
  ASSERT_TRUE(send_data.find("request handle timeout") != std::string::npos);
}

TEST_F(HttpAsyncStreamServiceTest, HandleUnaryMethodStreamMessageOk) {
  SetRecvSendFilterMockExpect();
  SetRpcFilterMockExpect();
  SetStreamHandlerMockExpect();
  // pushes stream frames one by one
  auto fut = RunAsMerge([&](FnProm prom) {
    server_stream_->PushRecvMessage(ConstructRequestLine("POST", "/hello"));
    server_stream_->PushRecvMessage(ConstructHeader({{"Content-Length", "16"}}));
    server_stream_->PushRecvMessage(ConstructData("hello stream msg"));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  // waits for asynchronous logic to finish processing
  std::this_thread::sleep_for(std::chrono::seconds(2));

  auto ft = RunAsMerge([&](FnProm prom) {
    std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());
    ASSERT_TRUE(send_data.find("HTTP/1.1 200 OK\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("Content-Length: 16\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("hello stream msg") != std::string::npos);
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
}

TEST_F(HttpAsyncStreamServiceTest, HandleUnaryMethodStreamMessageReadRequestTimeout) {
  SetRecvSendFilterMockExpect();
  SetStreamHandlerMockExpect();
  // pushes stream frames one by one
  auto fut = RunAsMerge([&](FnProm prom) {
    svr_ctx_->SetTimeout(100);
    server_stream_->PushRecvMessage(ConstructRequestLine("POST", "/hello"));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  // waits for asynchronous logic to finish processing
  std::this_thread::sleep_for(std::chrono::seconds(2));

  auto ft = RunAsMerge([&](FnProm prom) {
    std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());
    ASSERT_TRUE(send_data.find("HTTP/1.1 408 Request Timeout\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("Content-Length: 20\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("request wait timeout") != std::string::npos);
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
}

TEST_F(HttpAsyncStreamServiceTest, HandleUnaryMethodStreamMessageStreamError) {
  SetRecvSendFilterMockExpect();
  SetStreamHandlerMockExpect();
  // pushes stream frames one by one
  auto fut = RunAsMerge([&](FnProm prom) {
    // triggers error
    server_stream_->PushRecvMessage(ConstructRequestLine("POST", "/hello"));
    server_stream_->PushRecvMessage(ConstructData("hello stream msg"));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  // waits for asynchronous logic to finish processing
  std::this_thread::sleep_for(std::chrono::seconds(2));

  auto ft = RunAsMerge([&](FnProm prom) {
    std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());
    ASSERT_TRUE(send_data.find("HTTP/1.1 500 Internal Server Error\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("Content-Length: 14\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("internal error") != std::string::npos);
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
}

TEST_F(HttpAsyncStreamServiceTest, HandleStreamMethodWithFullMessageOk) {
  SetRecvSendFilterMockExpect();
  SetRpcFilterMockExpect();
  SetStreamHandlerMockExpect();
  // pushes a full message
  server_stream_->PushRecvMessage(
      ConstructRequestFullMessage("POST", "/hello-stream", {{"Content-Length", "14"}}, "hello full msg"));
  ASSERT_EQ(FlattenSlow(mock_stream_handler_->GetSendMessage()),
            "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\nhello full msg");
}

TEST_F(HttpAsyncStreamServiceTest, HandleStreamMethodWithStreamMessageOk) {
  SetRecvSendFilterMockExpect();
  SetRpcFilterMockExpect();
  SetStreamHandlerMockExpect();
  // pushes stream frames one by one
  auto fut = RunAsMerge([&](FnProm prom) {
    server_stream_->PushRecvMessage(ConstructRequestLine("POST", "/hello-stream"));
    server_stream_->PushRecvMessage(ConstructHeader({{"Content-Length", "16"}}));
    server_stream_->PushRecvMessage(ConstructData("hello stream msg"));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  // pushes stream frames one by one
  std::this_thread::sleep_for(std::chrono::seconds(2));

  auto ft = RunAsMerge([&](FnProm prom) {
    ASSERT_EQ(FlattenSlow(mock_stream_handler_->GetSendMessage()),
              "HTTP/1.1 200 OK\r\nContent-Length: 16\r\n\r\nhello stream msg");
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
}

TEST_F(HttpAsyncStreamServiceTest, HandleStreamMethodTimeoutError) {
  SetRecvSendFilterMockExpect();
  SetRpcFilterMockExpect();
  SetStreamHandlerMockExpect();
  // pushes stream frames one by one
  auto fut = RunAsMerge([&](FnProm prom) {
    server_stream_->PushRecvMessage(ConstructRequestLine("POST", "/hello-stream"));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  // waits for asynchronous logic to finish processing
  std::this_thread::sleep_for(std::chrono::seconds(2));

  auto ft = RunAsMerge([&](FnProm prom) {
    std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());
    ASSERT_TRUE(send_data.find("HTTP/1.1 408 Request Timeout\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("Content-Length: 48\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("ret: 254, func_ret: 0, err: read message timeout") != std::string::npos);
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
}

TEST_F(HttpAsyncStreamServiceTest, HandleStreamMethodStreamError) {
  SetRecvSendFilterMockExpect();
  SetRpcFilterMockExpect();
  SetStreamHandlerMockExpect();
  // pushes stream frames one by one
  auto fut = RunAsMerge([&](FnProm prom) {
    // triggers error
    server_stream_->PushRecvMessage(ConstructRequestLine("POST", "/hello-stream"));
    server_stream_->PushRecvMessage(ConstructData("hello stream msg"));
    prom->SetValue();
  });
  future::BlockingGet(std::move(fut));
  // waits for asynchronous logic to finish processing
  std::this_thread::sleep_for(std::chrono::seconds(2));

  auto ft = RunAsMerge([&](FnProm prom) {
    std::string send_data = FetchAllDataFromAdapterSendQueue(transport_.get());
    ASSERT_TRUE(send_data.find("HTTP/1.1 500 Internal Server Error\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("Content-Length: 14\r\n") != std::string::npos);
    ASSERT_TRUE(send_data.find("internal error") != std::string::npos);
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
}

}  // namespace trpc::testing
