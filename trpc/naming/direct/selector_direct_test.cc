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

#include "trpc/naming/direct/selector_direct.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"

namespace trpc {

TEST(SelectorDirect, select_test) {
  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  std::shared_ptr<SelectorDirect> ptr = std::make_shared<SelectorDirect>(polling);

  ptr->Init();
  ptr->Start();
  EXPECT_TRUE(ptr->Version() != "");

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1001;
  endpoint2.host = "127.0.0.1";
  endpoint2.port = 1002;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint2);
  ptr->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  ptr->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.host == "127.0.0.1") << (endpoint.host);
  EXPECT_TRUE(endpoint.port == 1001);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  ptr->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.host == "127.0.0.1");
  EXPECT_TRUE(endpoint.port == 1002);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  InvokeResult result;
  result.framework_result = 0;
  result.interface_result = 0;
  result.cost_time = 100;
  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  result.context = MakeRefCounted<ClientContext>(trpc_codec);
  result.context->SetCallerName("test_service");
  result.context->SetAddr("127.0.0.1", 1001);
  int ret = ptr->ReportInvokeResult(&result);
  EXPECT_EQ(0, ret);

  EXPECT_TRUE(ptr->ReportInvokeResult(nullptr) != 0);

  std::vector<TrpcEndpointInfo> endpoints;
  ptr->SelectBatch(&select_info, &endpoints);
  EXPECT_EQ(2, endpoints.size());

  // test select multiple
  endpoints.clear();
  select_info.policy = SelectorPolicy::MULTIPLE;
  ret = ptr->SelectBatch(&select_info, &endpoints);
  EXPECT_EQ(0, ret);

  select_info.name = "test_service1";
  ret = ptr->Select(&select_info, &endpoint);
  EXPECT_NE(0, ret);

  endpoints.clear();
  ret = ptr->SelectBatch(&select_info, &endpoints);
  EXPECT_NE(0, ret);

  ptr->Stop();
  ptr->Destroy();
}

TEST(SelectorDirect, select_exception_test) {
  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  std::shared_ptr<SelectorDirect> ptr = std::make_shared<SelectorDirect>(polling);

  ptr->Init();
  EXPECT_EQ(-1, ptr->SetEndpoints(nullptr));

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  int ret = ptr->SetEndpoints(&info);
  EXPECT_EQ(0, ret);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  ret = ptr->SelectBatch(nullptr, nullptr);
  EXPECT_NE(0, ret);

  ptr->AsyncSelectBatch(nullptr).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return MakeReadyFuture<>();
  });

  ptr->Stop();
  ptr->Destroy();
}

TEST(SelectorDirect, asyncselect_test) {
  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  std::shared_ptr<SelectorDirect> ptr = std::make_shared<SelectorDirect>(polling);

  ptr->Init();
  ptr->Start();

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1001;
  endpoint2.host = "127.0.0.1";
  endpoint2.port = 1002;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint2);
  ptr->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  ptr->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    TrpcEndpointInfo endpoint = fut.GetValue0();
    EXPECT_TRUE(endpoint.host == "127.0.0.1");
    EXPECT_TRUE(endpoint.port == 1001);
    return trpc::MakeReadyFuture<>();
  });

  ptr->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    TrpcEndpointInfo endpoint = fut.GetValue0();
    EXPECT_TRUE(endpoint.host == "127.0.0.1");
    EXPECT_TRUE(endpoint.port == 1002);
    return trpc::MakeReadyFuture<>();
  });

  ptr->AsyncSelectBatch(&select_info).Then([this](trpc::Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
    EXPECT_EQ(2, endpoints.size());
    return trpc::MakeReadyFuture<>();
  });

  // test select multiple
  select_info.policy = SelectorPolicy::MULTIPLE;
  ptr->AsyncSelectBatch(&select_info).Then([this](trpc::Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
    EXPECT_EQ(2, endpoints.size());
    return trpc::MakeReadyFuture<>();
  });

  select_info.name = "test_service1";
  ptr->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return trpc::MakeReadyFuture<>();
  });

  ptr->AsyncSelectBatch(&select_info).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return trpc::MakeReadyFuture<>();
  });

  ptr->Stop();
  ptr->Destroy();
}

TEST(SelectorDirect, endpoint_unique_id_test) {
  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  std::shared_ptr<SelectorDirect> ptr = std::make_shared<SelectorDirect>(polling);

  ptr->Init();

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1001;
  endpoint2.host = "127.0.0.1";
  endpoint2.port = 1002;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint2);
  ptr->SetEndpoints(&info);
  SelectorInfo select_info1;
  select_info1.name = "test_service";
  auto context1 = trpc::MakeRefCounted<trpc::ClientContext>();
  select_info1.context = context1;
  select_info1.policy = SelectorPolicy::ALL;
  std::vector<TrpcEndpointInfo> result1;
  ptr->SelectBatch(&select_info1, &result1);

  // Reset endpoints info: put endpoint2 after
  TrpcEndpointInfo endpoint3;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1003;
  endpoints_info.pop_back();
  endpoints_info.push_back(endpoint3);
  endpoints_info.push_back(endpoint2);
  ptr->SetEndpoints(&info);

  SelectorInfo select_info2;
  select_info2.name = "test_service";
  auto context2 = trpc::MakeRefCounted<trpc::ClientContext>();
  select_info2.context = context2;
  select_info2.policy = SelectorPolicy::ALL;
  std::vector<TrpcEndpointInfo> result2;
  ptr->SelectBatch(&select_info2, &result2);
  // Verify that the id of the same node in the original server is the same
  ASSERT_EQ(result1.size() + 1, result2.size());
  for (auto i : result1) {
    for (auto j : result2) {
      if (i.host == j.host && i.port == j.port) {
        ASSERT_EQ(i.id, j.id);
      }
    }
  }
}

}  // namespace trpc
