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
#include "trpc/rpcz/filter/server_filter.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/trpc_filter.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/rpcz/util/rpcz_fixture.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/server_context.h"
#include "trpc/server/testing/service_adapter_testing.h"
#include "trpc/transport/common/transport_message.h"

namespace trpc::testing {

class RpczServerFilterTest : public ::testing::Test {
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
  void InitServerContext(trpc::ServerContextPtr& context) {
    trpc::ServicePtr service;
    trpc::STransportReqMsg req_msg = PackTransportReqMsg(service);
    context = req_msg.context;

    trpc::ServiceAdapterOption service_adapter_option = CreateServiceAdapterOption();
    ServiceAdapter* service_adapter = new ServiceAdapter(std::move(service_adapter_option));

    FillServiceAdapter(service_adapter, "trpc.test.helloworld.Greeter", service);
  }

  static bool SampleSuccFuntion(const trpc::ServerContextPtr& context) { return true; }

  static bool SampleFailFuntion(const trpc::ServerContextPtr& context) { return false; }

  static trpc::rpcz::RpczServerFilter filter_;
};

// Global server filter.
trpc::rpcz::RpczServerFilter RpczServerFilterTest::filter_;

/// @brief Test for Name.
TEST_F(RpczServerFilterTest, Name) { ASSERT_EQ(filter_.Name(), "rpcz"); }

/// @brief Test for GetFilterPoint.
TEST_F(RpczServerFilterTest, GetFilterPoint) {
  auto points = filter_.GetFilterPoint();
  ASSERT_EQ(points.size(), 10);

  ASSERT_EQ(points[0], FilterPoint::SERVER_POST_RECV_MSG);
}

/// @brief Test for SetSampleFunction.
TEST_F(RpczServerFilterTest, SetSampleFunction) {
  trpc::ServerContextPtr context;

  filter_.SetSampleFunction(SampleFailFuntion);
  ASSERT_FALSE(filter_.GetSampleFunction()(context));

  filter_.SetSampleFunction(SampleSuccFuntion);
  ASSERT_TRUE(filter_.GetSampleFunction()(context));
}

/// @brief Test user-defined sample function which always return false.
TEST_F(RpczServerFilterTest, SamplePointTestWithSampleFail) {
  trpc::ServerContextPtr context;
  InitServerContext(context);

  filter_.SetSampleFunction(SampleFailFuntion);
  ASSERT_FALSE(filter_.GetSampleFunction()(context));

  FilterStatus status = FilterStatus::CONTINUE;
  auto point = trpc::FilterPoint::SERVER_PRE_SCHED_RECV_MSG;
  filter_.operator()(status, point, context);
  auto* ptr = context->GetFilterData<trpc::rpcz::ServerRpczSpan>(trpc::rpcz::kTrpcServerRpczIndex);
  ASSERT_EQ(ptr->span, nullptr);
}

/// @brief Test user-defined sample function which always return true.
TEST_F(RpczServerFilterTest, SamplePointTestWithSampleSucc) {
  trpc::ServerContextPtr context;
  InitServerContext(context);

  filter_.SetSampleFunction(SampleSuccFuntion);
  ASSERT_TRUE(filter_.GetSampleFunction()(context));

  // First point.
  FilterStatus status = FilterStatus::CONTINUE;
  auto point = trpc::FilterPoint::SERVER_PRE_SCHED_RECV_MSG;
  filter_.operator()(status, point, context);
  auto* ptr = context->GetFilterData<trpc::rpcz::ServerRpczSpan>(trpc::rpcz::kTrpcServerRpczIndex);
  ASSERT_NE(ptr, nullptr);
  ASSERT_NE(ptr->span, nullptr);
  ASSERT_EQ(ptr->span->RemoteName(), "trpc.test.helloworld.client");

  // Second point.
  point = trpc::FilterPoint::SERVER_POST_SCHED_RECV_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->HandleRealUs(), 0);

  // Third point.
  point = trpc::FilterPoint::SERVER_PRE_RPC_INVOKE;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartCallbackRealUs(), 0);

  // Fourth point.
  point = trpc::FilterPoint::SERVER_POST_RPC_INVOKE;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->CallbackRealUs(), 0);

  // Fifth point.
  point = trpc::FilterPoint::SERVER_PRE_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartEncodeRealUs(), 0);

  // Sixth point.
  point = trpc::FilterPoint::SERVER_PRE_SCHED_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->StartSendRealUs(), 0);

  // Seventh point.
  point = trpc::FilterPoint::SERVER_POST_SCHED_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->SendRealUs(), 0);
  ASSERT_EQ(ptr->span->ErrorCode(), 0);

  // Eighth point.
  point = trpc::FilterPoint::SERVER_POST_IO_SEND_MSG;
  filter_.operator()(status, point, context);
  ASSERT_GT(ptr->span->SendDoneRealUs(), 0);
}

}  // namespace trpc::testing
#endif
