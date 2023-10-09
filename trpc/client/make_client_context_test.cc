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

#include "trpc/client/make_client_context.h"

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/client/testing/service_proxy_testing.h"
#include "trpc/util/time.h"

namespace trpc::testing {

// Get service proxy for test
TestServiceProxyPtr GetTestServiceProxy() {
  TestServiceProxyPtr service_proxy = std::make_shared<TestServiceProxy>();
  auto proxy_option = std::make_shared<ServiceProxyOption>();
  detail::SetDefaultOption(proxy_option);
  proxy_option->name = "trpc.test.helloworld.Greeter";
  proxy_option->codec_name = "trpc";
  service_proxy->SetServiceProxyOption(proxy_option);
  return service_proxy;
}

class MakeClientContextTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() { RegisterPlugins(); }

  static void TearDownTestCase() { UnregisterPlugins(); }

  void SetUp() override {}

  void TearDown() override {}
};

// MakeClientContext with trans_info case
TEST_F(MakeClientContextTestFixture, MakeClientContextNormal) {
  ServerContextPtr ctx = MakeRefCounted<ServerContext>();
  ProtocolPtr req_msg = std::make_shared<TrpcRequestProtocol>();
  ctx->SetRequestMsg(std::move(req_msg));
  ctx->SetMessageType(TrpcMessageType::TRPC_DYEING_MESSAGE);
  ctx->SetCalleeName("test");
  ctx->SetFuncName("trpc.test.helloworld.Greeter");
  ctx->AddReqTransInfo("key", "value");

  ServiceProxyPtr service_proxy = GetTestServiceProxy();
  auto client_context = MakeClientContext(ctx, service_proxy);
  ASSERT_TRUE(client_context != nullptr);
  ASSERT_EQ(client_context->GetCodecName(), "trpc");
  ASSERT_EQ(client_context->GetMessageType(), ctx->GetMessageType());
  ASSERT_EQ(client_context->GetCallerName(), ctx->GetCalleeName());
  ASSERT_EQ(client_context->GetCallerFuncName(), ctx->GetFuncName());
  auto trans_info = client_context->GetPbReqTransInfo();
  auto iter = trans_info.find("key");
  ASSERT_TRUE(iter != trans_info.end() && iter->second == "value");
}

// without trans_info case
TEST_F(MakeClientContextTestFixture, MakeClientContextWithoutTransInfo) {
  ServerContextPtr ctx = MakeRefCounted<ServerContext>();
  ctx->SetRequestMsg(std::make_shared<TrpcRequestProtocol>());
  ctx->AddReqTransInfo("key", "value");
  ServiceProxyPtr service_proxy = GetTestServiceProxy();
  auto client_context = MakeClientContext(ctx, service_proxy, false);
  ASSERT_TRUE(client_context != nullptr);
  auto trans_info = client_context->GetPbReqTransInfo();
  auto iter = trans_info.find("key");
  ASSERT_TRUE(iter == trans_info.end());
}

// timeout case
TEST_F(MakeClientContextTestFixture, Timeout) {
  ServerContextPtr ctx = MakeRefCounted<ServerContext>();
  ctx->SetRequestMsg(std::make_shared<TrpcRequestProtocol>());
  ctx->SetBeginTimestampUs(trpc::time::GetMicroSeconds());
  ctx->SetTimeout(1500);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  ServiceProxyPtr service_proxy_1 = GetTestServiceProxy();
  auto client_context = MakeClientContext(ctx, service_proxy_1);
  ASSERT_TRUE(client_context->GetTimeout() <= 500);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  ServiceProxyPtr service_proxy_2 = GetTestServiceProxy();
  client_context = MakeClientContext(ctx, service_proxy_2);
  ASSERT_TRUE(client_context->GetTimeout() == 0);

  ctx->SetBeginTimestampUs(trpc::time::GetMicroSeconds());
  ctx->SetTimeout(0);
  ServiceProxyPtr service_proxy_3 = GetTestServiceProxy();
  client_context = MakeClientContext(ctx, service_proxy_3);
  ASSERT_TRUE(client_context->GetTimeout() == 0);
}

// transparent case
TEST_F(MakeClientContextTestFixture, MakeTransparentClientContext) {
  ServerContextPtr ctx = MakeRefCounted<ServerContext>();
  ctx->SetRequestMsg(std::make_shared<TrpcRequestProtocol>());
  ctx->SetResponseMsg(std::make_shared<TrpcResponseProtocol>());
  ctx->SetBeginTimestampUs(trpc::time::GetMicroSeconds());
  ctx->SetCallType(1);
  ctx->SetCallerName("Caller");
  ctx->SetFuncName("FuncName");
  std::unordered_map<std::string, std::string> test_map;
  test_map["test"] = "test";
  ctx->SetRspTransInfo(test_map.begin(), test_map.end());

  ServiceProxyPtr service_proxy = GetTestServiceProxy();
  auto client_ctx = MakeTransparentClientContext(ctx, service_proxy);

  ASSERT_TRUE(client_ctx != nullptr);
  ASSERT_TRUE(client_ctx->GetCallType() == 1);
  ASSERT_EQ(client_ctx->GetCallerName(), std::string("Caller"));
  ASSERT_EQ(client_ctx->GetCalleeName(), std::string(""));
  ASSERT_EQ(client_ctx->GetFuncName(), std::string("FuncName"));
  ASSERT_TRUE(client_ctx->GetMessageType() == 0);
  ASSERT_TRUE(client_ctx->GetReqEncodeType() == 0);
  ASSERT_TRUE(client_ctx->IsTransparent());
}

TEST_F(MakeClientContextTestFixture, BackFillServerTransInfoInSingleThread) {
  ServiceProxyPtr service_proxy = GetTestServiceProxy();
  ClientContextPtr client_ctx = MakeClientContext(service_proxy);
  ProtocolPtr client_rsp_msg = std::make_shared<TrpcResponseProtocol>();
  client_rsp_msg->SetKVInfo("k1", "v1");
  client_ctx->SetResponse(client_rsp_msg);
  ASSERT_EQ(client_ctx->GetPbRspTransInfo().size(), 1);

  ServerContextPtr server_ctx = MakeRefCounted<ServerContext>();
  ProtocolPtr server_req_msg = std::make_shared<TrpcRequestProtocol>();
  server_req_msg->SetRequestId(1);
  server_ctx->SetRequestMsg(std::move(server_req_msg));

  ProtocolPtr server_rsp_msg = std::make_shared<TrpcResponseProtocol>();
  server_ctx->SetResponseMsg(std::move(server_rsp_msg));

  BackFillServerTransInfo(client_ctx, server_ctx);
  const auto& rsp_trans_info = server_ctx->GetPbRspTransInfo();

  ASSERT_EQ(rsp_trans_info.size(), 1);

  auto it = rsp_trans_info.find("k1");
  ASSERT_TRUE(it != rsp_trans_info.end());
  ASSERT_EQ(it->second, "v1");
}

TEST_F(MakeClientContextTestFixture, BackFillServerTransInfoInMutiThread) {
  ServiceProxyPtr service_proxy = GetTestServiceProxy();
  ClientContextPtr client_ctx = MakeClientContext(service_proxy);
  ProtocolPtr client_rsp_msg = std::make_shared<TrpcResponseProtocol>();
  client_rsp_msg->SetKVInfo("k1", "v1");
  client_rsp_msg->SetKVInfo("k2", "v2");
  client_ctx->SetResponse(client_rsp_msg);

  ServerContextPtr server_ctx = MakeRefCounted<ServerContext>();

  ProtocolPtr server_req_msg = std::make_shared<TrpcRequestProtocol>();
  server_req_msg->SetRequestId(1);
  server_ctx->SetRequestMsg(std::move(server_req_msg));
  ProtocolPtr server_rsp_msg = std::make_shared<TrpcResponseProtocol>();
  server_ctx->SetResponseMsg(std::move(server_rsp_msg));

  auto fun = [server_ctx, client_ctx]() { BackFillServerTransInfo(client_ctx, server_ctx); };

  std::thread t1(fun);
  std::thread t2(fun);

  t1.join();
  t2.join();

  const auto& rsp_trans_info = server_ctx->GetPbRspTransInfo();
  ASSERT_EQ(rsp_trans_info.size(), 2);

  auto it = rsp_trans_info.find("k1");
  ASSERT_TRUE(it != rsp_trans_info.end());
  ASSERT_EQ(it->second, "v1");

  it = rsp_trans_info.find("k2");
  ASSERT_TRUE(it != rsp_trans_info.end());
  ASSERT_EQ(it->second, "v2");
}

}  // namespace trpc::testing
