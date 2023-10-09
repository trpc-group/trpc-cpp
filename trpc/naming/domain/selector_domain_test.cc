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

#include "trpc/naming/domain/selector_domain.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"

namespace trpc {

TEST(SelectorDomainTest, select_test) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  SelectorDomainPtr ptr = MakeRefCounted<SelectorDomain>(polling);
  ptr->Init();
  ptr->Start();
  EXPECT_TRUE(ptr->Version() != "");

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  ptr->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  ptr->Select(&select_info, &endpoint);
  if (endpoint.is_ipv6) {
    EXPECT_TRUE(endpoint.host == "::1");
  } else {
    EXPECT_TRUE(endpoint.host == "127.0.0.1");
  }
  EXPECT_TRUE(endpoint.port == 1001);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  ptr->Select(&select_info, &endpoint);
  if (endpoint.is_ipv6) {
    EXPECT_TRUE(endpoint.host == "::1");
  } else {
    EXPECT_TRUE(endpoint.host == "127.0.0.1");
  }
  EXPECT_TRUE(endpoint.port == 1001);
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

  ptr->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    TrpcEndpointInfo endpoint = fut.GetValue0();
    if (endpoint.is_ipv6) {
      EXPECT_TRUE(endpoint.host == "::1");
    } else {
      EXPECT_TRUE(endpoint.host == "127.0.0.1");
    }
    EXPECT_TRUE(endpoint.port == 1001);
    EXPECT_TRUE(endpoint.id != kInvalidEndpointId);
    return MakeReadyFuture<>();
  });

  std::vector<TrpcEndpointInfo> endpoints;
  ptr->SelectBatch(&select_info, &endpoints);
  for (const auto& ref : endpoints) {
    if (ref.is_ipv6) {
      EXPECT_TRUE(ref.host == "::1");
    } else {
      EXPECT_TRUE(ref.host == "127.0.0.1");
    }
    EXPECT_TRUE(ref.port == 1001);
    EXPECT_TRUE(ref.id != kInvalidEndpointId);
  }

  endpoints.clear();
  select_info.policy = SelectorPolicy::MULTIPLE;
  ret = ptr->SelectBatch(&select_info, &endpoints);
  EXPECT_EQ(0, ret);
  EXPECT_TRUE(endpoints.size() > 0);

  select_info.name = "test_service1";
  ret = ptr->Select(&select_info, &endpoint);
  EXPECT_NE(0, ret);

  ptr->SelectBatch(&select_info, &endpoints);
  EXPECT_NE(0, ret);

  ptr->Stop();
  ptr->Destroy();

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

TEST(SelectorDomainTest, select_exception_test) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  SelectorDomainPtr ptr = MakeRefCounted<SelectorDomain>(polling);
  ptr->Init();
  ptr->Start();
  EXPECT_EQ(-1, ptr->SetEndpoints(nullptr));

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoint2.host = "localhost";
  endpoint2.port = 1002;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint2);

  int ret = ptr->SetEndpoints(&info);
  EXPECT_NE(0, ret);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  ret = ptr->Select(&select_info, &endpoint);
  EXPECT_NE(0, ret);

  ret = ptr->Select(nullptr, &endpoint);
  EXPECT_NE(0, ret);

  ptr->AsyncSelect(nullptr).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return MakeReadyFuture<>();
  });

  ret = ptr->SelectBatch(nullptr, nullptr);
  EXPECT_NE(0, ret);

  ptr->AsyncSelectBatch(nullptr).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return MakeReadyFuture<>();
  });

  ptr->Stop();
  ptr->Destroy();

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

TEST(SelectorDomainTest, ayncselect_test) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  SelectorDomainPtr ptr = MakeRefCounted<SelectorDomain>(polling);
  ptr->Init();
  ptr->Start();

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  ptr->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;

  ptr->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    TrpcEndpointInfo endpoint = fut.GetValue0();
    if (endpoint.is_ipv6) {
      EXPECT_TRUE(endpoint.host == "::1");
    } else {
      EXPECT_TRUE(endpoint.host == "127.0.0.1");
    }
    EXPECT_TRUE(endpoint.port == 1001);
    return MakeReadyFuture<>();
  });

  ptr->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    TrpcEndpointInfo endpoint = fut.GetValue0();
    if (endpoint.is_ipv6) {
      EXPECT_TRUE(endpoint.host == "::1");
    } else {
      EXPECT_TRUE(endpoint.host == "127.0.0.1");
    }
    EXPECT_TRUE(endpoint.port == 1001);
    return MakeReadyFuture<>();
  });

  ptr->AsyncSelectBatch(&select_info).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
    for (const auto& ref : endpoints) {
      if (ref.is_ipv6) {
        EXPECT_TRUE(ref.host == "::1");
      } else {
        EXPECT_TRUE(ref.host == "127.0.0.1");
      }
      EXPECT_TRUE(ref.port == 1001);
    }
    return trpc::MakeReadyFuture<>();
  });

  select_info.policy = SelectorPolicy::MULTIPLE;
  ptr->AsyncSelectBatch(&select_info).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
    EXPECT_TRUE(endpoints.size() > 0);
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

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

TEST(SelectorDomainTest, UpdateEndpointInfo) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  SelectorDomainPtr ptr = MakeRefCounted<SelectorDomain>(polling);
  ptr->Init();
  ptr->Start();
  EXPECT_TRUE(ptr->Version() != "");

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  ptr->SetEndpoints(&info);
  // Internal tasks perform node updates once in 200ms
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  ptr->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.port == 1001);

  ptr->Stop();
  ptr->Destroy();

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

TEST(SelectorDomainTest, exclude_ipv6_test) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  auto ret = trpc::TrpcConfig::GetInstance()->Init("./trpc/naming/testing/domain_test.yaml");
  ASSERT_EQ(0, ret);

  LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
  SelectorDomainPtr ptr = MakeRefCounted<SelectorDomain>(polling);
  ptr->Init();
  ptr->Start();

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  ASSERT_EQ(ptr->SetEndpoints(&info), 0);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;

  std::vector<TrpcEndpointInfo> endpoints;
  ASSERT_EQ(ptr->SelectBatch(&select_info, &endpoints), 0);

  int ipv4_num = 0;
  int ipv6_num = 0;
  int total_num = 0;
  for (const auto& ref : endpoints) {
    if (ref.is_ipv6) {
      ipv6_num++;
    } else {
      ipv4_num++;
    }
    total_num++;
    ASSERT_EQ(ref.port, 1001);
    ASSERT_NE(ref.id, kInvalidEndpointId);
  }

  ASSERT_NE(total_num, 0);
  ASSERT_TRUE(ipv4_num == total_num || ipv6_num == total_num);

  ptr->Stop();
  ptr->Destroy();

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

}  // namespace trpc
