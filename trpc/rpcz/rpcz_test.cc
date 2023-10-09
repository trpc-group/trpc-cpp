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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/rpcz.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/codec/trpc/trpc_server_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/rpcz_fixture.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/server_context.h"
#include "trpc/server/testing/service_adapter_testing.h"
#include "trpc/util/http/request.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/string_helper.h"

namespace trpc::testing {

/// @brief Text fixture.
class RpczTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/server/testing/merge_server.yaml"), 0);
    trpc::codec::Init();
    trpc::serialization::Init();
    trpc::merge::StartRuntime();
    trpc::separate::StartAdminRuntime();
  }

  static void TearDownTestCase() {
    trpc::separate::TerminateAdminRuntime();
    trpc::merge::TerminateRuntime();
    trpc::serialization::Destroy();
    trpc::codec::Destroy();
  }

 protected:
  /// @brief Init a server context with transport request message already decoded.
  /// @param context Server context to init.
  void InitServerContext(trpc::ServerContextPtr& context) {
    trpc::ServicePtr service;
    trpc::STransportReqMsg req_msg = PackTransportReqMsg(service);
    context = req_msg.context;

    trpc::ServiceAdapterOption service_adapter_option = CreateServiceAdapterOption();
    ServiceAdapter* service_adapter = new ServiceAdapter(std::move(service_adapter_option));

    FillServiceAdapter(service_adapter, "trpc.test.helloworld.Greeter", service);
  }
};

/// @brief Test for create server rpcz span.
TEST_F(RpczTest, CreateServerRpczSpanTest) {
  trpc::ServerContextPtr context;
  InitServerContext(context);

  const uint64_t recv_timestamp_us = trpc::GetSystemMicroSeconds();
  trpc::rpcz::Span* span_ptr = trpc::rpcz::CreateServerRpczSpan(context, 1, recv_timestamp_us);

  ASSERT_EQ(span_ptr->ProtocolName(), "trpc");
  ASSERT_EQ(span_ptr->RemoteName(), "trpc.test.helloworld.client");
  ASSERT_GT(span_ptr->RequestSize(), 0);
}

TEST_F(RpczTest, CreateUserRpczSpanWithContext) {
  trpc::ServerContextPtr context;
  InitServerContext(context);

  trpc::rpcz::Span* span_ptr = trpc::rpcz::CreateUserRpczSpan("with_context", context);
  ASSERT_NE(span_ptr, nullptr);
  ASSERT_EQ(span_ptr->ViewerName(), "with_context");
  ASSERT_NE(trpc::rpcz::GetUserRpczSpan(context), nullptr);

  trpc::rpcz::SubmitUserRpczSpan(span_ptr);
  ASSERT_EQ(span_ptr->EndViewerEvent().type, trpc::rpcz::PhaseType::kPhaseTypeEnd);
}

TEST_F(RpczTest, CreateUserRpczSpanWithoutContext) {
  trpc::rpcz::Span* span_ptr = trpc::rpcz::CreateUserRpczSpan("without_context");
  ASSERT_NE(span_ptr, nullptr);
  ASSERT_EQ(span_ptr->ViewerName(), "without_context");

  trpc::ServerContextPtr context;
  InitServerContext(context);
  ASSERT_EQ(trpc::rpcz::GetUserRpczSpan(context), nullptr);
}

/// @brief Test for admin query without parameter.
TEST_F(RpczTest, TryGetTestWithNoParamsTest) {
  // Without span info.
  auto http_req = std::make_shared<trpc::http::HttpRequest>();
  http_req->SetUrl("'http://0.0.0.0:11111/cmds/rpcz");
  std::string get_result = trpc::rpcz::TryGet(http_req);
  ASSERT_EQ(get_result, "");
}

/// @brief Test for admin query with span id set.
TEST_F(RpczTest, TryGetTestWithParamsTest) {
  auto http_req = std::make_shared<trpc::http::HttpRequest>();
  http_req->SetUrl("'http://0.0.0.0:11111/cmds/rpcz?span_id=1");
  std::string get_result = trpc::rpcz::TryGet(http_req);
  ASSERT_EQ(get_result, "");
}

/// @brief Test for RpczPrint with null context.
TEST_F(RpczTest, RpczPrintTestWithContextNullptr) {
  trpc::ServerContextPtr context{nullptr};
  trpc::rpcz::RpczPrint(context, "rpcz/test.cc", 10, "hello rpcz");
  ASSERT_FALSE(context);
}

/// @brief Test for RpczPrint.
TEST_F(RpczTest, RpczPrintTest) {
  trpc::ServerContextPtr context;
  InitServerContext(context);
  const uint64_t recv_timestamp_us = trpc::GetSystemMicroSeconds();
  trpc::rpcz::Span* span_ptr = trpc::rpcz::CreateServerRpczSpan(context, 1, recv_timestamp_us);
  trpc::rpcz::ServerRpczSpan svr_rpcz_span;
  svr_rpcz_span.span = span_ptr;
  context->SetFilterData<trpc::rpcz::ServerRpczSpan>(trpc::rpcz::kTrpcServerRpczIndex, std::move(svr_rpcz_span));

  trpc::rpcz::RpczPrint(context, "rpcz/test.cc", 10, "hello rpcz");

  auto* ptr = context->GetFilterData<trpc::rpcz::ServerRpczSpan>(trpc::rpcz::kTrpcServerRpczIndex);
  ASSERT_TRUE(ptr && ptr->span);
  ASSERT_EQ(ptr->span->SubSpans().size(), 1);

  std::string log = ptr->span->SubSpans().back()->CustomLogs();
  auto vec = trpc::Split(log, '[');
  ASSERT_EQ(2, vec.size());
  ASSERT_EQ("rpcz/test.cc:10] hello rpcz\n", vec[1]);
}

}  // namespace trpc::testing
#endif
