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

#include "trpc/server/forward/forward_rpc_service_impl.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/forward/forward_rpc_method_handler.h"
#include "trpc/server/testing/server_context_testing.h"

namespace trpc::testing {

class ForwardRpcServiceImplTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    codec::Init();
    serialization::Init();
  }

  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
  }
};

class Greeter {
 public:
  trpc::Status SayHello(trpc::ServerContextPtr context, NoncontiguousBuffer&& request, NoncontiguousBuffer& reply) {
    reply = std::move(request);
    return context->GetStatus();
  }
};

TEST_F(ForwardRpcServiceImplTest, Normal) {
  NoncontiguousBuffer hello_req = CreateBufferSlow("hello world");

  DummyTrpcProtocol req_data;
  req_data.data_type = serialization::kNonContiguousBufferNoop;
  req_data.content_type = TrpcContentEncodeType::TRPC_NOOP_ENCODE;
  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<ForwardRpcServiceImpl> test_forword_rpc_server_impl = std::make_shared<ForwardRpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_forword_rpc_server_impl.get(), std::move(req_bin_data));

  Greeter greeter;

  test_forword_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      kTransparentRpcName, trpc::MethodType::UNARY,
      new trpc::ForwardRpcMethodHandler(std::bind(&Greeter::SayHello, &greeter,
          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))));

  test_forword_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(context->GetStatus().OK());

  NoncontiguousBuffer hello_rsp;

  NoncontiguousBuffer rsp_bin_data = context->GetResponseMsg()->GetNonContiguousProtocolBody();

  std::cout << "pack begin" << std::endl;

  ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));

  std::string rsp = trpc::FlattenSlow(hello_rsp);

  ASSERT_TRUE(rsp == "hello world");
}

TEST_F(ForwardRpcServiceImplTest, NotFoundFunc) {
  NoncontiguousBuffer hello_req = CreateBufferSlow("hello world");

  DummyTrpcProtocol req_data;
  req_data.data_type = serialization::kNonContiguousBufferNoop;
  req_data.content_type = TrpcContentEncodeType::TRPC_NOOP_ENCODE;
  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<ForwardRpcServiceImpl> test_forword_rpc_server_impl = std::make_shared<ForwardRpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_forword_rpc_server_impl.get(),
      std::move(req_bin_data));

  test_forword_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(!context->GetStatus().OK());
  ASSERT_TRUE(context->GetStatus().ErrorMessage() == "_transparent_rpc_ method handler not register");
}

}  // namespace trpc::testing
