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

#include "trpc/client/http/http_stream_proxy.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/codec/http/http_client_codec.h"
#include "trpc/codec/http/http_server_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/testing/client_filter_testing.h"
#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/testing/service_adapter_testing.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

using namespace stream;

class MockHttpStreamProxy : public HttpStreamProxy {
 public:
  MockHttpStreamProxy() {
    option_ = std::make_shared<ServiceProxyOption>();
    trpc::detail::SetDefaultOption(option_);
    option_->name = "default_http_service";
    option_->caller_name = "";
    option_->codec_name = "http";
    option_->conn_type = "long";
    option_->network = "tcp";
    option_->timeout = 1000;
    option_->target = "127.0.0.1:10002";
    option_->selector_name = "direct";

    codec_ = std::make_shared<HttpClientCodec>();
    SetServiceProxyOptionInner(option_);
  }
  TransInfo ProxyOptionToTransInfo() override { return HttpStreamProxy::ProxyOptionToTransInfo(); }

  void AddClientFilter(const MessageClientFilterPtr& filter) { filter_controller_.AddMessageClientFilter(filter); }

  MOCK_METHOD(StreamReaderWriterProviderPtr, SelectStreamProvider, (const ClientContextPtr&, void*), (override));

 private:
  std::shared_ptr<ServiceProxyOption> option_{nullptr};
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

class HttpStreamProxyTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml");
    merge::StartRuntime();
    codec::Init();
    serialization::Init();
  }

  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
    merge::TerminateRuntime();
  }

  void SetUp() override {
    mock_stream_proxy_ = std::make_shared<MockHttpStreamProxy>();
    client_ctx_ = MakeClientContext(mock_stream_proxy_);

    mock_filter_ = std::make_shared<MockClientFilter>();
    std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_PRE_SEND_MSG,
                                       FilterPoint::CLIENT_POST_RECV_MSG, FilterPoint::CLIENT_POST_RPC_INVOKE};
    EXPECT_CALL(*mock_filter_, Name()).WillRepeatedly(::testing::Return("test_filters"));
    EXPECT_CALL(*mock_filter_, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
    mock_stream_proxy_->AddClientFilter(mock_filter_);
  }

  void TearDown() override {}

 protected:
  std::shared_ptr<MockHttpStreamProxy> mock_stream_proxy_{nullptr};
  std::shared_ptr<MockClientFilter> mock_filter_{nullptr};
  ClientContextPtr client_ctx_{nullptr};
};

using FnProm = std::shared_ptr<Promise<>>;
Future<> RunAsMerge(std::function<void(FnProm)> fn) {
  static auto thread_model = static_cast<MergeThreadModel*>(merge::RandomGetMergeThreadModel());
  auto prom = std::make_shared<Promise<>>();
  thread_model->GetReactor(1)->SubmitTask([prom, fn]() { fn(prom); });
  return prom->GetFuture();
}

TEST_F(HttpStreamProxyTest, TestStreamFlagSet) {
  ASSERT_TRUE(mock_stream_proxy_->ProxyOptionToTransInfo().is_stream_proxy);
}

TEST_F(HttpStreamProxyTest, TestInvokeByMainThread) {
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_SEND_MSG, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_RPC_INVOKE, ::testing::_)).Times(1);

  StreamReaderWriterProviderPtr provider = MakeRefCounted<HttpClientAsyncStream>(StreamOptions{});
  EXPECT_CALL(*mock_stream_proxy_, SelectStreamProvider(::testing::_, ::testing::_))
      .WillOnce(::testing::Return(provider));

  Future<HttpClientAsyncStreamReaderWriterPtr> fut = mock_stream_proxy_->GetAsyncStreamReaderWriter(client_ctx_);
  fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.IsReady());
}

TEST_F(HttpStreamProxyTest, TestInvokeByWorkerThread) {
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_SEND_MSG, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_RPC_INVOKE, ::testing::_)).Times(1);

  StreamReaderWriterProviderPtr provider = MakeRefCounted<HttpClientAsyncStream>(StreamOptions{});
  EXPECT_CALL(*mock_stream_proxy_, SelectStreamProvider(::testing::_, ::testing::_))
      .WillOnce(::testing::Return(provider));

  Future<HttpClientAsyncStreamReaderWriterPtr> fut;
  auto ft = RunAsMerge([&](FnProm prom) {
    fut = mock_stream_proxy_->GetAsyncStreamReaderWriter(client_ctx_);
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
  future::BlockingGet(std::move(fut));
}

TEST_F(HttpStreamProxyTest, TestInvokeByMainThreadStreamNotOk) {
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_SEND_MSG, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_RPC_INVOKE, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_POST_RPC_INVOKE, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_POST_RECV_MSG, ::testing::_)).Times(1);

  EXPECT_CALL(*mock_stream_proxy_, SelectStreamProvider(::testing::_, ::testing::_))
      .WillOnce(::testing::Return(CreateErrorStreamProvider(Status{233, 0, "233-test-error"})));

  Future<HttpClientAsyncStreamReaderWriterPtr> fut = mock_stream_proxy_->GetAsyncStreamReaderWriter(client_ctx_);
  fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.IsFailed());
  ASSERT_EQ(fut.GetException().GetExceptionCode(), 233);
}

TEST_F(HttpStreamProxyTest, TestInvokeByWorkerThreadNotOk) {
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_SEND_MSG, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_RPC_INVOKE, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_POST_RPC_INVOKE, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_POST_RECV_MSG, ::testing::_)).Times(1);

  EXPECT_CALL(*mock_stream_proxy_, SelectStreamProvider(::testing::_, ::testing::_))
      .WillOnce(::testing::Return(CreateErrorStreamProvider(Status{233, 0, "233-test-error"})));

  Future<HttpClientAsyncStreamReaderWriterPtr> fut;
  auto ft = RunAsMerge([&](FnProm prom) {
    fut = mock_stream_proxy_->GetAsyncStreamReaderWriter(client_ctx_);
    prom->SetValue();
  });
  future::BlockingGet(std::move(ft));
  ASSERT_TRUE(fut.IsFailed());
  ASSERT_EQ(fut.GetException().GetExceptionCode(), 233);
}

TEST_F(HttpStreamProxyTest, TestPreRpcInvokeReject) {
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_RPC_INVOKE, ::testing::_))
      .WillOnce(::testing::SetArgReferee<0>(FilterStatus::REJECT));
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_POST_RPC_INVOKE, ::testing::_)).Times(0);

  Future<HttpClientAsyncStreamReaderWriterPtr> fut = mock_stream_proxy_->GetAsyncStreamReaderWriter(client_ctx_);
  fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.IsFailed());
  ASSERT_EQ(fut.GetException().GetExceptionCode(), TRPC_STREAM_UNKNOWN_ERR);
}

TEST_F(HttpStreamProxyTest, TestPreSendMsgReject) {
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_SEND_MSG, ::testing::_))
      .WillOnce(::testing::SetArgReferee<0>(FilterStatus::REJECT));
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_POST_RECV_MSG, ::testing::_)).Times(0);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_PRE_RPC_INVOKE, ::testing::_)).Times(1);
  EXPECT_CALL(*mock_filter_, BracketOp(::testing::_, FilterPoint::CLIENT_POST_RPC_INVOKE, ::testing::_)).Times(1);

  Future<HttpClientAsyncStreamReaderWriterPtr> fut = mock_stream_proxy_->GetAsyncStreamReaderWriter(client_ctx_);
  fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.IsFailed());
  ASSERT_EQ(fut.GetException().GetExceptionCode(), TRPC_STREAM_UNKNOWN_ERR);
}

}  // namespace trpc::testing
