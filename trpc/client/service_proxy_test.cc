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

#include "trpc/client/service_proxy.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/client/testing/mock_client_transport.h"
#include "trpc/client/testing/service_proxy_testing.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/filter/client_filter_manager.h"
#include "trpc/future/future_utility.h"
#include "trpc/naming/trpc_naming.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/stream/testing/mock_stream_handler.h"

namespace trpc::testing {

class ServiceProxyTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    RegisterPlugins();

    detail::SetDefaultOption(default_option_);
    default_option_->name = "trpc.test.helloworld.Greeter";
    default_option_->caller_name = "Test.HelloWorldClient";
    default_option_->codec_name = "trpc";
    default_option_->conn_type = "long";
    default_option_->network = "tcp";
    default_option_->timeout = 1000;
    default_option_->target = "127.0.0.1:8000";
    default_option_->callee_name = "trpc.test.helloworld.Greeter";
    default_option_->selector_name = "direct";
    default_option_->proxy_callback.create_transport_function = []() {
      auto transport = std::make_unique<MockClientTransport>();
      if (mock_transport_ == nullptr) {
        mock_transport_ = transport.get();
      }
      return transport;
    };

    test_service_proxy_ = std::make_shared<TestServiceProxy>();
    auto option = std::make_shared<ServiceProxyOption>();
    *option = *default_option_;
    test_service_proxy_->SetServiceProxyOption(option);
    ASSERT_TRUE(mock_transport_ != nullptr);
  }

  static void TearDownTestCase() {
    mock_transport_ = nullptr;
    test_service_proxy_->Stop();
    test_service_proxy_->Destroy();
    UnregisterPlugins();
  }

  void FillBackRequestAddrs(ClientContextPtr& context) {
    std::vector<NodeAddr> addrs;
    addrs.emplace_back(NodeAddr{.addr_type = {}, .connection_type = {}, .port = 10001, .ip = "127.0.0.1"});
    addrs.emplace_back(NodeAddr{.addr_type = {}, .connection_type = {}, .port = 10002, .ip = "127.0.0.1"});
    context->SetBackupRequestAddrs(addrs);
  }

 protected:
  static std::shared_ptr<ServiceProxyOption> default_option_;
  static TestServiceProxyPtr test_service_proxy_;
  static MockClientTransport* mock_transport_;
};

std::shared_ptr<ServiceProxyOption> ServiceProxyTestFixture::default_option_ = std::make_shared<ServiceProxyOption>();
TestServiceProxyPtr ServiceProxyTestFixture::test_service_proxy_ = nullptr;
MockClientTransport* ServiceProxyTestFixture::mock_transport_ = nullptr;

// SelectTarget
TEST_F(ServiceProxyTestFixture, SelectTarget) {
  TrpcRouterInfo info;
  info.plugin_name = "direct";
  info.router_info.name = test_service_proxy_->GetServiceProxyOption()->name;
  info.router_info.info.push_back(TrpcEndpointInfo{.host = "127.0.0.1", .port = 8080});
  naming::SetEndpoints(info);
  auto context = MakeClientContext(test_service_proxy_);
  test_service_proxy_->FillClientContext(context);
  int ret = test_service_proxy_->RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(!context->GetIp().empty());
  ASSERT_TRUE(context->GetPort() != 0);
}

// GetServiceProxyOption: GetServiceName/CodecName
TEST_F(ServiceProxyTestFixture, GetServiceProxyOption) {
  const ServiceProxyOption* option = test_service_proxy_->GetServiceProxyOption();
  ASSERT_TRUE(option != nullptr);
  ASSERT_EQ(test_service_proxy_->GetServiceName(), option->name);
  ASSERT_EQ(test_service_proxy_->GetClientCodec()->Name(), "trpc");
}

// UnaryInvoke OK
TEST_F(ServiceProxyTestFixture, UnaryInvokeOK) {
  CTransportRspMsg rsp_msg;
  rsp_msg.msg = std::static_pointer_cast<Protocol>(std::make_shared<TrpcResponseProtocol>());
  EXPECT_CALL(*mock_transport_, SendRecv(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(rsp_msg), ::testing::Return(0)));

  auto context = MakeClientContext(test_service_proxy_);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  Status status = test_service_proxy_->UnaryInvoke(context, req, rsp);
  ASSERT_TRUE(status.OK());
}

// UnaryInvoke Failed
TEST_F(ServiceProxyTestFixture, UnaryInvokeFailed) {
  EXPECT_CALL(*mock_transport_, SendRecv(::testing::_, ::testing::_))
      .WillOnce(::testing::Return(TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR));

  auto context = MakeClientContext(test_service_proxy_);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  Status status = test_service_proxy_->UnaryInvoke(context, req, rsp);
  ASSERT_TRUE(!status.OK());
  ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
}

// AsyncUnaryInvoke OK
TEST_F(ServiceProxyTestFixture, AsyncUnaryInvokeOK) {
  CTransportRspMsg rsp_msg;
  rsp_msg.msg = std::static_pointer_cast<Protocol>(std::make_shared<TrpcResponseProtocol>());
  EXPECT_CALL(*mock_transport_, AsyncSendRecv(::testing::_))
      .WillOnce(::testing::Return(::testing::ByMove(std::move(MakeReadyFuture<CTransportRspMsg>(std::move(rsp_msg))))));

  auto context = MakeClientContext(test_service_proxy_);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  auto fut = test_service_proxy_->AsyncUnaryInvoke(context, req);
  fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.IsReady());
}

// AsyncUnaryInvoke failed
TEST_F(ServiceProxyTestFixture, AsyncUnaryInvokeFailed) {
  EXPECT_CALL(*mock_transport_, AsyncSendRecv(::testing::_))
      .WillOnce(::testing::Return(::testing::ByMove(MakeExceptionFuture<CTransportRspMsg>(
          CommonException("Timeout", TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR)))));

  auto context = MakeClientContext(test_service_proxy_);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  auto fut = test_service_proxy_->AsyncUnaryInvoke(context, req);
  fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.IsFailed());
  ASSERT_EQ(fut.GetException().GetExceptionCode(), TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
}

// OnewayInvoke OK
TEST_F(ServiceProxyTestFixture, OnewayInvokeOK) {
  CTransportRspMsg rsp_msg;
  rsp_msg.msg = std::static_pointer_cast<Protocol>(std::make_shared<TrpcResponseProtocol>());
  EXPECT_CALL(*mock_transport_, SendOnly(::testing::_)).WillOnce(::testing::Return(0));

  auto context = MakeClientContext(test_service_proxy_);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  Status status = test_service_proxy_->OnewayInvoke(context, req);
  ASSERT_TRUE(status.OK());
}

// OnewayInvoke failed
TEST_F(ServiceProxyTestFixture, OnewayInvokeFailed) {
  CTransportRspMsg rsp_msg;
  rsp_msg.msg = std::static_pointer_cast<Protocol>(std::make_shared<TrpcResponseProtocol>());
  EXPECT_CALL(*mock_transport_, SendOnly(::testing::_))
      .WillOnce(::testing::Return(TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR));

  auto context = MakeClientContext(test_service_proxy_);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  Status status = test_service_proxy_->OnewayInvoke(context, req);
  ASSERT_TRUE(!status.OK());
  ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR);
}

// Check Timeout
TEST_F(ServiceProxyTestFixture, CheckNormalTimeout) {
  auto context = MakeClientContext(test_service_proxy_);
  context->SetTimeout(0);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  Status status = test_service_proxy_->UnaryInvoke(context, req, rsp);
  ASSERT_TRUE(!status.OK());
  ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
}

// Check FullLink Timeout
TEST_F(ServiceProxyTestFixture, CheckFullLinkTimeout) {
  auto context = MakeClientContext(test_service_proxy_);
  context->SetFullLinkTimeout(0);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  Status status = test_service_proxy_->UnaryInvoke(context, req, rsp);
  ASSERT_TRUE(!status.OK());
  ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR);
}

// fixed connection
TEST_F(ServiceProxyTestFixture, GetAndReleaseFixedConnector) {
  PreallocationOption preallocate_option;
  preallocate_option.node_addr.addr_type = NodeAddr::AddrType::kIpV4;
  preallocate_option.node_addr.ip = "127.0.0.1";
  preallocate_option.node_addr.port = 10000;
  EXPECT_CALL(*mock_transport_, GetOrCreateConnector(::testing::_)).WillOnce(::testing::Return(1));
  auto conn_id = test_service_proxy_->GetOrCreateConnector(preallocate_option);
  ASSERT_EQ(*conn_id, 1);
  EXPECT_CALL(*mock_transport_, ReleaseFixedConnector(::testing::_)).WillOnce(::testing::Return(true));
  ASSERT_TRUE(test_service_proxy_->ReleaseFixedConnector(*conn_id));
}

// Select stream provider OK
TEST_F(ServiceProxyTestFixture, SelectStreamProviderOK) {
  auto context = MakeClientContext(test_service_proxy_);
  context->SetAddr("127.0.0.1", 10001);

  stream::StreamReaderWriterProviderPtr provider = MakeRefCounted<stream::ErrorStreamProvider>();
  EXPECT_CALL(*mock_transport_, CreateStream(::testing::_, ::testing::_)).Times(1)
      .WillOnce(::testing::Return(provider));
  auto stream_provider = test_service_proxy_->MockSelectStreamProvider(context, nullptr);
  ASSERT_TRUE(stream_provider != nullptr);
  ASSERT_TRUE(context->GetStatus().OK());
}

// Select stream provider failed
TEST_F(ServiceProxyTestFixture, SelectStreamProviderFailed) {
  auto context = MakeClientContext(test_service_proxy_);
  context->SetAddr("127.0.0.1", 10001);

  EXPECT_CALL(*mock_transport_, CreateStream(::testing::_, ::testing::_)).WillOnce(::testing::Return(nullptr));
  auto stream_provider = test_service_proxy_->MockSelectStreamProvider(context, nullptr);
  ASSERT_TRUE(stream_provider != nullptr);
  const auto& status = context->GetStatus();
  ASSERT_FALSE(status.OK()) << status.ToString();
  ASSERT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_STREAM_CLIENT_NETWORK_ERR);
}

// Degraded to a normal unary invoke when the backup request delay time if no less than request timeout
TEST_F(ServiceProxyTestFixture, BackRequestWithBigDelayTime) {
  CTransportRspMsg rsp_msg;
  rsp_msg.msg = std::static_pointer_cast<Protocol>(std::make_shared<TrpcResponseProtocol>());
  auto context = MakeClientContext(test_service_proxy_);
  context->SetBackupRequestDelay(test_service_proxy_->GetServiceProxyOption()->timeout);
  ASSERT_EQ(context->IsBackupRequest(), true);
  test_service_proxy_->FillClientContext(context);
  ASSERT_EQ(context->IsBackupRequest(), false);
}

// Synchronous backup request
TEST_F(ServiceProxyTestFixture, SyncBackRequestOK) {
  CTransportRspMsg rsp_msg;
  rsp_msg.msg = std::static_pointer_cast<Protocol>(std::make_shared<TrpcResponseProtocol>());
  EXPECT_CALL(*mock_transport_, SendRecv(::testing::_, ::testing::_))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<1>(rsp_msg), ::testing::Return(0)));

  auto context = MakeClientContext(test_service_proxy_);
  context->SetBackupRequestDelay(test_service_proxy_->GetServiceProxyOption()->timeout);
  FillBackRequestAddrs(context);
  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  Status status = test_service_proxy_->UnaryInvoke(context, req, rsp);
  ASSERT_TRUE(status.OK());
  ASSERT_EQ(context->IsBackupRequest(), true);
  auto backup_request_stat =
      FrameStats::GetInstance()->GetBackupRequestStats().GetData(test_service_proxy_->GetServiceProxyOption()->name);
  ASSERT_TRUE(backup_request_stat.retries->GetValue() == 0);
}

// Asynchronous backup request
TEST_F(ServiceProxyTestFixture, AsyncBackRequest) {
  CTransportRspMsg rsp_msg;
  rsp_msg.msg = std::static_pointer_cast<Protocol>(std::make_shared<TrpcResponseProtocol>());
  auto context = MakeClientContext(test_service_proxy_);
  context->SetBackupRequestDelay(test_service_proxy_->GetServiceProxyOption()->timeout);
  FillBackRequestAddrs(context);

  EXPECT_CALL(*mock_transport_, AsyncSendRecv(::testing::_))
      .WillOnce(::testing::DoAll(
          ::testing::Invoke([retry_info = context->GetBackupRequestRetryInfo()](CTransportReqMsg*) {
            retry_info->resend_count++;
            retry_info->succ_rsp_node_index = 1;
          }),
          ::testing::Return(::testing::ByMove(std::move(MakeReadyFuture<CTransportRspMsg>(std::move(rsp_msg)))))));

  TrpcRequestProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
  testing::FillTrpcRequestProtocolData(*req);
  ProtocolPtr rsp;
  auto fut = test_service_proxy_->AsyncUnaryInvoke(context, req);
  fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.IsReady());
  ASSERT_EQ(context->IsBackupRequest(), true);
  auto backup_request_stat =
      FrameStats::GetInstance()->GetBackupRequestStats().GetData(test_service_proxy_->GetServiceProxyOption()->name);
  ASSERT_TRUE(backup_request_stat.retries->GetValue() > 0);
  ASSERT_TRUE(backup_request_stat.retries_success->GetValue() > 0);
}

// test service filter without shared data
struct TestNoSharedDataFilterConfig {
  bool reject = false;
};

class TestNoSharedDataFilter : public MessageClientFilter {
 public:
  explicit TestNoSharedDataFilter(const TestNoSharedDataFilterConfig* config) {
    if (config != nullptr) {
      config_ = *config;
    }
  }

  std::string Name() override { return "test_no_shared_data"; }

  virtual std::vector<FilterPoint> GetFilterPoint() override {
    return std::vector<FilterPoint>{FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  }

  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override {
    if (config_.reject) {
      status = FilterStatus::REJECT;
    }
  }

  MessageClientFilterPtr Create(const std::any& param) override {
    TestNoSharedDataFilterConfig config;
    if (param.has_value()) {
      config = std::any_cast<TestNoSharedDataFilterConfig>(param);
    }

    return std::make_shared<TestNoSharedDataFilter>(&config);
  }

 private:
  TestNoSharedDataFilterConfig config_;
};

TEST_F(ServiceProxyTestFixture, NoSharedDataFilter) {
  MessageClientFilterPtr filter = std::make_shared<TestNoSharedDataFilter>(nullptr);
  ClientFilterManager::GetInstance()->AddMessageClientFilter(filter);
  TestNoSharedDataFilterConfig config;
  config.reject = true;

  TestServiceProxy prx;
  auto proxy_option = std::make_shared<ServiceProxyOption>();
  *proxy_option = *default_option_;
  proxy_option->service_filters.push_back(filter->Name());
  proxy_option->service_filter_configs[filter->Name()] = config;
  prx.SetServiceProxyOption(proxy_option);

  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  int ret = prx.RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, client_context);
  ASSERT_TRUE(ret == -1);
}

// specify the selector filter
TEST_F(ServiceProxyTestFixture, SpecifiedSelectorFilter) {
  class FixedIpSelectorFilter : public MessageClientFilter {
   public:
    FixedIpSelectorFilter() = default;

    ~FixedIpSelectorFilter() override {}

    std::string Name() override { return "fixed_ip"; }

    std::vector<FilterPoint> GetFilterPoint() override {
      std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
      return points;
    }

    void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override {
      if (point == FilterPoint::CLIENT_PRE_RPC_INVOKE) {
        ExtendNodeAddr addr;
        addr.addr.ip = "127.0.0.1";
        addr.addr.port = 10001;
        context->SetRequestAddrByNaming(std::move(addr));
        status = FilterStatus::CONTINUE;
      }
    }
  };

  // register filter
  ClientFilterManager::GetInstance()->AddMessageClientFilter(std::make_shared<FixedIpSelectorFilter>());

  // execute filter
  auto prx = std::make_shared<TestServiceProxy>();
  auto proxy_option = std::make_shared<ServiceProxyOption>();
  *proxy_option = *default_option_;
  proxy_option->selector_name = "fixed_ip";
  prx->SetServiceProxyOption(proxy_option);
  ClientContextPtr context = MakeClientContext(prx);
  int ret = prx->RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  ASSERT_EQ(ret, 0);
  ASSERT_EQ(context->GetIp(), "127.0.0.1");
  ASSERT_EQ(context->GetPort(), 10001);
  prx->Stop();
  prx->Destroy();
}

}  // namespace trpc::testing
