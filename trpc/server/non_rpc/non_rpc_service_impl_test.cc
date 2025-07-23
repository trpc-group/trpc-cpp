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

#include "trpc/server/non_rpc/non_rpc_service_impl.h"

#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/codec/testing/server_codec_testing.h"
#include "trpc/server/non_rpc/non_rpc_method_handler.h"
#include "trpc/server/server_context.h"
#include "trpc/server/testing/server_context_testing.h"

namespace trpc::testing {

class TestNonRpcServerImpl : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ServerCodecFactory::GetInstance()->Register(std::make_shared<TestServerCodec>());
  }

  static void TearDownTestCase() {}
};

static trpc::Status SayHello1(trpc::ServerContextPtr context, const TestProtocol* request, TestProtocol* reply) {
  return trpc::Status(0, "");
}

static trpc::Status SayHello2(trpc::ServerContextPtr context, const TestProtocolPtr& request, TestProtocolPtr& reply) {
  return trpc::Status(0, "");
}

TEST_F(TestNonRpcServerImpl, Normal1) {
  std::shared_ptr<NonRpcServiceImpl> test_non_rpc_server_impl = std::make_shared<NonRpcServiceImpl>();
  std::any any_data;

  ServerContextPtr context = MakeTestServerContext("TestServerCodec", test_non_rpc_server_impl.get(),
      std::move(any_data));
  context->SetFuncName("_non_rpc_1");

  test_non_rpc_server_impl->AddNonRpcServiceMethod(
      new trpc::NonRpcServiceMethod("_non_rpc_1", trpc::MethodType::UNARY,
          new trpc::NonRpcMethodHandler<TestProtocol, TestProtocol>(SayHello1)));

  test_non_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  ASSERT_EQ(true, context->GetStatus().OK());
}

TEST_F(TestNonRpcServerImpl, Normal2) {
  std::shared_ptr<NonRpcServiceImpl> test_non_rpc_server_impl = std::make_shared<NonRpcServiceImpl>();
  std::any any_data;

  ServerContextPtr context = MakeTestServerContext("TestServerCodec", test_non_rpc_server_impl.get(),
      std::move(any_data));
  context->SetFuncName("_non_rpc_2");

  test_non_rpc_server_impl->AddNonRpcServiceMethod(
      new trpc::NonRpcServiceMethod("_non_rpc_2", trpc::MethodType::UNARY,
          new trpc::NonRpcMethodHandler<TestProtocol, TestProtocol>(SayHello2)));

  test_non_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  ASSERT_EQ(true, context->GetStatus().OK());
}

TEST_F(TestNonRpcServerImpl, NotFoundFunc) {
  std::shared_ptr<NonRpcServiceImpl> test_non_rpc_server_impl = std::make_shared<NonRpcServiceImpl>();
  std::any any_data;

  ServerContextPtr context = MakeTestServerContext("TestServerCodec", test_non_rpc_server_impl.get(),
      std::move(any_data));
  context->SetFuncName("_non_rpc_3");

  test_non_rpc_server_impl->AddNonRpcServiceMethod(
      new trpc::NonRpcServiceMethod("_non_rpc_4", trpc::MethodType::UNARY,
          new trpc::NonRpcMethodHandler<TestProtocol, TestProtocol>(SayHello2)));

  test_non_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  ASSERT_TRUE(!context->GetStatus().OK());
  ASSERT_TRUE(context->GetStatus().ErrorMessage() == "_non_rpc_3 not found");
}

}  // namespace trpc::testing
