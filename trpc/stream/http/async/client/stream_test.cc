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

#include "trpc/stream/http/async/client/stream.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/testing/client_filter_testing.h"
#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using namespace trpc::stream;

class MockHttpClientAsyncStream : public HttpClientAsyncStream {
 public:
  using State = HttpCommonStream::State;

  explicit MockHttpClientAsyncStream(StreamOptions&& options) : HttpClientAsyncStream(std::move(options)) {}

  void SetReadState(State state) { read_state_ = state; }
  State GetReadState() { return read_state_; }
  void SetWriteState(State state) { write_state_ = state; }
  State GetWriteState() { return write_state_; }
  void SetStop(bool is_stop) { SetStopped(is_stop); }

  int GetReadTimeoutErrorCode() override { return HttpClientAsyncStream::GetReadTimeoutErrorCode(); }

  int GetWriteTimeoutErrorCode() override { return HttpClientAsyncStream::GetWriteTimeoutErrorCode(); }

  int GetNetWorkErrorCode() override { return HttpClientAsyncStream::GetNetWorkErrorCode(); }
};
using State = MockHttpClientAsyncStream::State;
using MockHttpClientAsyncStreamPtr = RefPtr<MockHttpClientAsyncStream>;

class TestMockStreamHandler : public MockStreamHandler {
 public:
  int SendMessage(const std::any& data, NoncontiguousBuffer&& buff) override {
    send_buf.Append(buff);
    return 0;
  }

 public:
  NoncontiguousBuffer send_buf;
};

class HttpClientAsyncStreamTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml");
    merge::StartRuntime();
  }

  static void TearDownTestCase() { merge::TerminateRuntime(); }

  void SetUp() override {
    StreamOptions options;
    options.stream_id = 1;
    mock_stream_handler_ = MakeRefCounted<TestMockStreamHandler>();
    options.stream_handler = mock_stream_handler_;
    options.callbacks.on_close_cb = [&](int reason) { close_trigger_flag_ = true; };
    options.context.context = MakeRefCounted<ClientContext>();
    mock_stream_ = MakeRefCounted<MockHttpClientAsyncStream>(std::move(options));
  }

  void TearDown() override {}

 protected:
  MockHttpClientAsyncStreamPtr mock_stream_{nullptr};
  RefPtr<TestMockStreamHandler> mock_stream_handler_{nullptr};
  bool close_trigger_flag_ = false;
};

using FnProm = std::shared_ptr<Promise<>>;
Future<> RunAsMerge(std::function<void(FnProm)> fn) {
  static auto thread_model = static_cast<MergeThreadModel*>(merge::RandomGetMergeThreadModel());
  auto prom = std::make_shared<Promise<>>();
  thread_model->GetReactor(1)->SubmitTask([prom, fn]() { fn(prom); });
  return prom->GetFuture();
}

TEST_F(HttpClientAsyncStreamTest, TestErrorCode) {
  ASSERT_EQ(mock_stream_->GetReadTimeoutErrorCode(), TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR);
  ASSERT_EQ(mock_stream_->GetWriteTimeoutErrorCode(), TRPC_STREAM_CLIENT_WRITE_TIMEOUT_ERR);
  ASSERT_EQ(mock_stream_->GetNetWorkErrorCode(), TRPC_STREAM_CLIENT_NETWORK_ERR);
}

TEST_F(HttpClientAsyncStreamTest, TestRecvStartLine) {
  HttpStatusLine check_line;
  // Response is ready before reading.
  auto fut1 = RunAsMerge([&](FnProm prom) {
    auto start_line = MakeRefCounted<HttpStreamStatusLine>();
    start_line->GetMutableHttpStatusLine()->status_code = 202;
    start_line->GetMutableHttpStatusLine()->status_text = "OK2";
    start_line->GetMutableHttpStatusLine()->version = "1.1";
    mock_stream_->PushRecvMessage(start_line);
    mock_stream_->ReadStatusLine().Then([&, prom](HttpStatusLine&& line) {
      check_line = line;
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut1));
  ASSERT_EQ(check_line.status_code, 202);
  ASSERT_EQ(check_line.status_text, "OK2");
  ASSERT_EQ(check_line.version, "1.1");
  ASSERT_EQ(mock_stream_->GetReadState(), State::kInit);

  // Response is ready after reading.
  check_line = HttpStatusLine();
  auto fut2 = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kIdle);
    mock_stream_->ReadStatusLine().Then([&, prom](HttpStatusLine&& line) {
      check_line = line;
      prom->SetValue();
    });
    auto start_line = MakeRefCounted<HttpStreamStatusLine>();
    start_line->GetMutableHttpStatusLine()->status_code = 202;
    start_line->GetMutableHttpStatusLine()->status_text = "OK2";
    start_line->GetMutableHttpStatusLine()->version = "1.1";
    mock_stream_->PushRecvMessage(start_line);
  });
  future::BlockingGet(std::move(fut2));
  ASSERT_EQ(check_line.status_code, 202);
  ASSERT_EQ(check_line.status_text, "OK2");
  ASSERT_EQ(check_line.version, "1.1");

  // Reading error occurred.
  auto mock_filter = std::make_shared<MockClientFilter>();
  std::vector<FilterPoint> points = {FilterPoint::CLIENT_POST_RECV_MSG, FilterPoint::CLIENT_POST_RPC_INVOKE};
  EXPECT_CALL(*mock_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
  EXPECT_CALL(*mock_filter, Name()).WillRepeatedly(::testing::Return("test_client_filter"));
  ClientFilterController filter_controller;
  bool add_ok = filter_controller.AddMessageClientFilter(mock_filter);
  std::cerr << "add filter: " << add_ok << std::endl;
  mock_stream_->SetFilterController(&filter_controller);
  EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1);
  int err_code = 0;
  auto fut3 = RunAsMerge([&](FnProm prom) {
    mock_stream_->SetReadState(State::kIdle);
    mock_stream_->ReadStatusLine().Then([&, prom](Future<HttpStatusLine>&& fut) {
      err_code = fut.GetException().GetExceptionCode();
      prom->SetValue();
    });

    mock_stream_->PushRecvMessage(MakeRefCounted<HttpStreamHeader>());
  });
  future::BlockingGet(std::move(fut3));
  ASSERT_EQ(err_code, TRPC_STREAM_UNKNOWN_ERR);
  ASSERT_EQ(mock_stream_->GetStatus().GetFrameworkRetCode(), TRPC_STREAM_UNKNOWN_ERR);
  ASSERT_TRUE(close_trigger_flag_);
}

TEST_F(HttpClientAsyncStreamTest, TestSendStartLine) {
  auto fut = RunAsMerge([&](FnProm prom) {
    auto start_line = MakeRefCounted<HttpStreamRequestLine>();
    start_line->GetMutableHttpRequestLine()->version = "1.1";
    start_line->GetMutableHttpRequestLine()->method = "POST";
    start_line->GetMutableHttpRequestLine()->request_uri = "/hello";
    mock_stream_->PushSendMessage(start_line).Then([&, prom]() {
      prom->SetValue();
    });
  });
  future::BlockingGet(std::move(fut));
  ASSERT_EQ(FlattenSlow(mock_stream_handler_->send_buf), "POST /hello HTTP/1.1\r\n");
  ASSERT_EQ(mock_stream_->GetWriteState(), State::kInit);
}

TEST_F(HttpClientAsyncStreamTest, TestFinish) {
  auto mock_filter = std::make_shared<MockClientFilter>();
  std::vector<FilterPoint> points = {FilterPoint::CLIENT_POST_RECV_MSG, FilterPoint::CLIENT_POST_RPC_INVOKE};
  EXPECT_CALL(*mock_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
  EXPECT_CALL(*mock_filter, Name()).WillRepeatedly(::testing::Return("test_client_filter"));

  ClientFilterController filter_controller;
  filter_controller.AddMessageClientFilter(mock_filter);
  mock_stream_->SetFilterController(&filter_controller);

//  EXPECT_CALL(*mock_filter, BracketOp(::testing::_, ::testing::_, ::testing::_)).Times(2);
  EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kClosed);
  mock_stream_->SetWriteState(State::kClosed);
  Status status = mock_stream_->Finish();
  ASSERT_TRUE(status.OK());
  ASSERT_TRUE(close_trigger_flag_);
  close_trigger_flag_ = false;

  mock_stream_->SetStop(false);
//  EXPECT_CALL(*mock_filter, BracketOp(::testing::_, ::testing::_, ::testing::_)).Times(2);
  EXPECT_CALL(*mock_stream_handler_, RemoveStream(::testing::_)).Times(1);
  mock_stream_->SetReadState(State::kIdle);
  mock_stream_->SetWriteState(State::kIdle);
  status = mock_stream_->Finish();
  ASSERT_FALSE(status.OK());
  ASSERT_TRUE(close_trigger_flag_);
  ASSERT_EQ(status.GetFrameworkRetCode(), TRPC_STREAM_UNKNOWN_ERR);
}

}  // namespace trpc::testing
