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

#include "trpc/naming/testing/selector_loadbalance_test.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/naming/common/util/loadbalance/hash/consistenthash_load_balance.h"
#include "trpc/naming/common/util/loadbalance/hash/modulohash_load_balance.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"

namespace trpc {

TEST(TestSelectorLoadBalance, polling_test) {
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<ConsistentHashLoadBalance>());
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<ModuloHashLoadBalance>());
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<PollingLoadBalance>());
  auto r = trpc::TrpcConfig::GetInstance()->Init("./trpc/naming/testing/loadbalance_test.yaml");
  ASSERT_EQ(0, r);
  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  std::shared_ptr<TestSelectorLoadBalance> ptr = std::make_shared<TestSelectorLoadBalance>(polling);

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
  endpoint2.host = "192.168.0.2";
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
  EXPECT_TRUE(endpoint.host == "192.168.0.2");
  EXPECT_TRUE(endpoint.port == 1002);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  InvokeResult result;
  result.framework_result = 0;
  result.interface_result = 0;
  result.cost_time = 100;
  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  result.context = MakeRefCounted<ClientContext>(trpc_codec);
  result.context->SetCallerName("test_service");
  result.context->SetAddr("192.168.0.1", 1001);
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

TEST(TestSelectorLoadBalance, consistenthash_test) {
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<ConsistentHashLoadBalance>());
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<ModuloHashLoadBalance>());
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<PollingLoadBalance>());
  auto r = trpc::TrpcConfig::GetInstance()->Init("./trpc/naming/testing/consistenthash_test.yaml");
  ASSERT_EQ(0, r);
  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  std::shared_ptr<TestSelectorLoadBalance> ptr = std::make_shared<TestSelectorLoadBalance>(polling);

  ptr->Init();
  ptr->Start();

  EXPECT_TRUE(ptr->loadbalance_config_.load_balance_name == "trpc_consistenthash_load_balance");
  EXPECT_TRUE(ptr->loadbalance_config_.hash_func == "murmur3");
  EXPECT_TRUE(ptr->loadbalance_config_.hash_nodes == 1);
  EXPECT_TRUE(ptr->loadbalance_config_.hash_args.size() == 3);

  EXPECT_TRUE(ptr->Version() != "");

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1001;
  endpoint2.host = "192.168.0.2";
  endpoint2.port = 1002;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint2);
  ptr->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;
  TrpcEndpointInfo temp;

  ptr->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.host == "127.0.0.1" || endpoint.host == "192.168.0.2") << (endpoint.host);
  EXPECT_TRUE(endpoint.port == 1001 || endpoint.port == 1002);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  ptr->Select(&select_info, &temp);
  EXPECT_TRUE((temp.host == "192.168.0.2" || temp.host == "127.0.0.1")&& temp.host != endpoint.host);
  EXPECT_TRUE((temp.port == 1002 || temp.port == 1001) && temp.port != endpoint.port);
  EXPECT_TRUE(temp.id != kInvalidEndpointId);

  InvokeResult result;
  result.framework_result = 0;
  result.interface_result = 0;
  result.cost_time = 100;
  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  result.context = MakeRefCounted<ClientContext>(trpc_codec);
  result.context->SetCallerName("test_service");
  result.context->SetAddr("192.168.0.1", 1001);
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

TEST(TestSelectorLoadBalance, modulohash_test) {
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<ConsistentHashLoadBalance>());
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<ModuloHashLoadBalance>());
  TrpcPlugin::GetInstance()->RegisterLoadBalance(MakeRefCounted<PollingLoadBalance>());
  auto r = trpc::TrpcConfig::GetInstance()->Init("./trpc/naming/testing/modulohash_test.yaml");
  ASSERT_EQ(0, r);
  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  std::shared_ptr<TestSelectorLoadBalance> ptr = std::make_shared<TestSelectorLoadBalance>(polling);

  ptr->Init();
  ptr->Start();

  EXPECT_TRUE(ptr->loadbalance_config_.load_balance_name == "trpc_modulohash_load_balance");
  EXPECT_TRUE(ptr->loadbalance_config_.hash_func == "md5");
  EXPECT_TRUE(ptr->loadbalance_config_.hash_nodes == 20);
  EXPECT_TRUE(ptr->loadbalance_config_.hash_args.size() == 2);

  EXPECT_TRUE(ptr->Version() != "");

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1001;
  endpoint2.host = "192.168.0.2";
  endpoint2.port = 1002;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint2);
  ptr->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;
  TrpcEndpointInfo temp;

  ptr->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.host == "127.0.0.1" || endpoint.host == "192.168.0.2") << (endpoint.host);
  EXPECT_TRUE(endpoint.port == 1001 || endpoint.port == 1002);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  ptr->Select(&select_info, &temp);
  EXPECT_TRUE((temp.host == "192.168.0.2" || temp.host == "127.0.0.1")&& temp.host != endpoint.host);
  EXPECT_TRUE((temp.port == 1002 || temp.port == 1001) && temp.port != endpoint.port);
  EXPECT_TRUE(temp.id != kInvalidEndpointId);

  InvokeResult result;
  result.framework_result = 0;
  result.interface_result = 0;
  result.cost_time = 100;
  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  result.context = MakeRefCounted<ClientContext>(trpc_codec);
  result.context->SetCallerName("test_service");
  result.context->SetAddr("192.168.0.1", 1001);
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

}