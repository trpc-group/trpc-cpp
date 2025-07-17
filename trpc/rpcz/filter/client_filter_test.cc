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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/filter/client_filter.h"

#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/client/testing/service_proxy_testing.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/trpc_filter.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/rpcz/rpcz.h"
#include "trpc/rpcz/util/rpcz_fixture.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/server_context.h"
#include "trpc/server/testing/service_adapter_testing.h"
#include "trpc/transport/common/transport_message.h"

namespace trpc::testing {

class RpczClientFilterTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/server/testing/merge_server.yaml"), 0);
    trpc::codec::Init();
    trpc::serialization::Init();
    trpc::merge::StartRuntime();
    trpc::separate::StartAdminRuntime();
    trpc::filter::Init();
    filter_.Init();
  }

  static void TearDownTestCase() {
    trpc::separate::TerminateAdminRuntime();
    trpc::merge::TerminateRuntime();
    trpc::serialization::Destroy();
    trpc::codec::Destroy();
  }

 protected:
  void InitClientContext(trpc::ClientContextPtr& context) {
    trpc::ProtocolPtr req_msg = std::make_shared<trpc::TrpcRequestProtocol>();
    context->SetRequest(std::move(req_msg));
    context->SetCallType(trpc::kUnaryCall);
    context->SetAddr("0.0.0.0", 10008);
    context->SetCalleeName("trpc.test.helloworld.greeter");
  }

  void InitServerContext(trpc::ServerContextPtr& context, bool sample_flag) {
    trpc::ServicePtr service;
    trpc::STransportReqMsg req_msg = PackTransportReqMsg(service);
    context = req_msg.context;

    trpc::ServiceAdapterOption service_adapter_option = CreateServiceAdapterOption();
    ServiceAdapter* service_adapter = new ServiceAdapter(std::move(service_adapter_option));

    FillServiceAdapter(service_adapter, "trpc.test.helloworld.Greeter", service);

    trpc::rpcz::ServerRpczSpan svr_rpcz_span;
    svr_rpcz_span.sample_flag = sample_flag;
    if (sample_flag) {
      trpc::rpcz::Span* span_ptr = trpc::rpcz::CreateServerRpczSpan(context, 1, trpc::GetSystemMicroSeconds());
      svr_rpcz_span.span = span_ptr;
    } else {
      svr_rpcz_span.span = nullptr;
    }

    context->SetFilterData<trpc::rpcz::ServerRpczSpan>(trpc::rpcz::kTrpcServerRpczIndex, std::move(svr_rpcz_span));
  }

  static bool SampleSuccFuntion(const trpc::ClientContextPtr& context) { return true; }

  static bool SampleFailFuntion(const trpc::ClientContextPtr& context) { return false; }

  static trpc::rpcz::RpczClientFilter filter_;
};

trpc::rpcz::RpczClientFilter RpczClientFilterTest::filter_;

/// @brief Test for Name.
TEST_F(RpczClientFilterTest, Name) { ASSERT_EQ(filter_.Name(), "rpcz"); }

/// @brief Test for GetFilterPoint.
TEST_F(RpczClientFilterTest, GetFilterPoint) {
  auto points = filter_.GetFilterPoint();
  ASSERT_EQ(points.size(), 10);

  ASSERT_EQ(points[0], FilterPoint::CLIENT_PRE_SEND_MSG);
}

/// @brief Test user-defined sample function which always return false.
TEST_F(RpczClientFilterTest, SamplePointTestWithSampleFail) {
  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  auto context = MakeRefCounted<trpc::ClientContext>(trpc_codec);
  InitClientContext(context);

  filter_.SetSampleFunction(SampleFailFuntion);
  ASSERT_FALSE(filter_.GetSampleFunction()(context));

  FilterStatus status = FilterStatus::CONTINUE;
  auto point = trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE;
  filter_.operator()(status, point, context);
  auto* ptr = context->GetFilterData<trpc::rpcz::ClientRpczSpan>(trpc::rpcz::kTrpcClientRpczIndex);
  ASSERT_EQ(ptr->span, nullptr);
}

/// @brief Test user-defined sample function which always return true.
TEST_F(RpczClientFilterTest, SamplePointTestWithSampleSucc) {
  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  auto context = MakeRefCounted<trpc::ClientContext>(trpc_codec);
  InitClientContext(context);

  filter_.SetSampleFunction(SampleSuccFuntion);
  ASSERT_TRUE(filter_.GetSampleFunction()(context));

  // First point.
  FilterStatus status = FilterStatus::CONTINUE;
  auto point = trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE;
  filter_.operator()(status, point, context);

  auto* ptr = context->GetFilterData<trpc::rpcz::ClientRpczSpan>(trpc::rpcz::kTrpcClientRpczIndex);
  ASSERT_NE(ptr, nullptr);
  ASSERT_NE(ptr->span, nullptr);
  ASSERT_EQ(ptr->span->CallType(), trpc::kUnaryCall);
  ASSERT_EQ(ptr->span->ProtocolName(), "trpc");
  ASSERT_EQ(ptr->span->RemoteName(), "trpc.test.helloworld.greeter");
  ASSERT_EQ(ptr->span->Type(), trpc::rpcz::SpanType::kSpanTypeClient);
  ASSERT_GT(ptr->span->FirstLogRealUs(), 0);
  ASSERT_GT(ptr->span->StartRpcInvokeRealUs(), 0);
  ASSERT_NE(ptr->span->RemoteName(), "0.0.0.0:10008");

  // Second point, transpaort invoke.
  point = trpc::FilterPoint::CLIENT_PRE_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartTransInvokeRealUs(), 0);

  // Third point, before into send queue.
  point = trpc::FilterPoint::CLIENT_PRE_SCHED_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartSendRealUs(), 0);

  // Fourth point, after out send queue, before writev.
  point = trpc::FilterPoint::CLIENT_POST_SCHED_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->SendRealUs(), 0);

  // Fifth point, after writev.
  point = trpc::FilterPoint::CLIENT_POST_IO_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->SendDoneRealUs(), 0);

  // Sixth point, before response into recv queue.
  point = trpc::FilterPoint::CLIENT_PRE_SCHED_RECV_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartHandleRealUs(), 0);

  // Seventh point, after response out recv queue.
  point = trpc::FilterPoint::CLIENT_POST_SCHED_RECV_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->HandleRealUs(), 0);

  // Eighth point, transport finish handle.
  point = trpc::FilterPoint::CLIENT_POST_RECV_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->TransInvokeDoneRealUs(), 0);

  // Ninth point, finish.
  point = trpc::FilterPoint::CLIENT_POST_RPC_INVOKE;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->RpcInvokeDoneRealUs(), 0);
  ASSERT_EQ(ptr->span->ErrorCode(), 0);
  ASSERT_GT(ptr->span->LastLogRealUs(), 0);
}

/// @brief Test for route scenario.
TEST_F(RpczClientFilterTest, RouteSamplePointTest) {
  trpc::ServerContextPtr server_context;
  InitServerContext(server_context, true);

  auto proxy_option = std::make_shared<trpc::ServiceProxyOption>();
  trpc::detail::SetDefaultOption(proxy_option);
  proxy_option->name = "trpc.test.helloworld.Greeter";
  proxy_option->codec_name = "trpc";

  std::shared_ptr<TestServiceProxy> test_prx = std::make_shared<TestServiceProxy>();
  test_prx->SetServiceProxyOption(proxy_option);
  auto context = trpc::MakeClientContext(server_context, test_prx);

  trpc::ProtocolPtr trpc_req = std::make_shared<trpc::TrpcRequestProtocol>();
  context->SetRequest(std::move(trpc_req));

  filter_.SetSampleFunction(SampleSuccFuntion);
  ASSERT_TRUE(filter_.GetSampleFunction()(context));

  FilterStatus status = FilterStatus::CONTINUE;
  auto point = trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE;
  filter_.operator()(status, point, context);
  auto* ptr = context->GetFilterData<trpc::rpcz::ClientRouteRpczSpan>(trpc::rpcz::kTrpcRouteRpczIndex);
  ASSERT_NE(ptr, nullptr);
  ASSERT_NE(ptr->parent_span, nullptr);
  ASSERT_EQ(ptr->span->Type(), trpc::rpcz::SpanType::kSpanTypeClient);
  ASSERT_GT(ptr->span->FirstLogRealUs(), 0);
  ASSERT_GT(ptr->span->StartRpcInvokeRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_PRE_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartTransInvokeRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_PRE_SCHED_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartSendRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_POST_SCHED_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->SendRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_POST_IO_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->SendDoneRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_PRE_SCHED_RECV_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartHandleRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_POST_SCHED_RECV_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->HandleRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_POST_RECV_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->TransInvokeDoneRealUs(), 0);

  point = trpc::FilterPoint::CLIENT_POST_RPC_INVOKE;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->RpcInvokeDoneRealUs(), 0);
  ASSERT_EQ(ptr->span->ErrorCode(), 0);
  ASSERT_GT(ptr->span->LastLogRealUs(), 0);
}

}  // namespace trpc::testing
#endif
