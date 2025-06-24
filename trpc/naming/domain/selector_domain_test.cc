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
#include "trpc/naming/common/util/circuit_break/circuit_breaker_creator_factory.h"
#include "trpc/naming/common/util/circuit_break/default_circuit_breaker.h"
#include "trpc/naming/common/util/circuit_break/default_circuit_breaker_config.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/domain/domain_selector_conf_parser.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/util/domain_util.h"

namespace trpc {

class SelectorDomainTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    // register circuitbreak creator
    naming::CircuitBreakerCreatorFactory::GetInstance()->Register(
        "default", [](const YAML::Node* plugin_config, const std::string& service_name) {
          naming::DefaultCircuitBreakerConfig config;
          if (plugin_config) {
            YAML::convert<naming::DefaultCircuitBreakerConfig>::decode(*plugin_config, config);
          }
          return std::make_shared<naming::DefaultCircuitBreaker>(config, service_name);
        });

    PeripheryTaskScheduler::GetInstance()->Init();
    PeripheryTaskScheduler::GetInstance()->Start();
  }

  static void TearDownTestCase() {
    PeripheryTaskScheduler::GetInstance()->Stop();
    PeripheryTaskScheduler::GetInstance()->Join();
  }

  void SetUp() override {
    int ret = TrpcConfig::GetInstance()->Init("./trpc/naming/testing/domain_selector_test.yaml");
    ASSERT_TRUE(ret == 0);
    naming::DomainSelectorConfig config;
    trpc::TrpcConfig::GetInstance()->GetPluginConfig("selector", "domain", config);
    YAML::convert<naming::DefaultCircuitBreakerConfig>::decode(config.circuit_break_config.plugin_config,
                                                               circuit_break_config_);

    LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
    domain_selector_ = std::make_shared<SelectorDomain>(polling);
    domain_selector_->Init();
    domain_selector_->Start();
  }

  void TearDown() override {
    domain_selector_->Stop();
    domain_selector_->Destroy();
  }

 protected:
  std::shared_ptr<SelectorDomain> domain_selector_;
  naming::DefaultCircuitBreakerConfig circuit_break_config_;
};

TEST_F(SelectorDomainTest, select_test) {
  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  domain_selector_->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  domain_selector_->Select(&select_info, &endpoint);
  if (endpoint.is_ipv6) {
    EXPECT_TRUE(endpoint.host == "::1");
  } else {
    EXPECT_TRUE(endpoint.host == "127.0.0.1");
  }
  EXPECT_TRUE(endpoint.port == 1001);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  domain_selector_->Select(&select_info, &endpoint);
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
  ExtendNodeAddr addr;
  addr.addr.ip = "127.0.0.1";
  addr.addr.port = 1001;
  result.context->SetRequestAddrByNaming(std::move(addr));
  int ret = domain_selector_->ReportInvokeResult(&result);
  EXPECT_EQ(0, ret);

  EXPECT_TRUE(domain_selector_->ReportInvokeResult(nullptr) != 0);

  domain_selector_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
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
  domain_selector_->SelectBatch(&select_info, &endpoints);
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
  ret = domain_selector_->SelectBatch(&select_info, &endpoints);
  EXPECT_EQ(0, ret);
  EXPECT_TRUE(endpoints.size() > 0);

  select_info.name = "test_service1";
  ret = domain_selector_->Select(&select_info, &endpoint);
  EXPECT_NE(0, ret);

  domain_selector_->SelectBatch(&select_info, &endpoints);
  EXPECT_NE(0, ret);
}

TEST_F(SelectorDomainTest, select_exception_test) {
  EXPECT_EQ(-1, domain_selector_->SetEndpoints(nullptr));

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

  int ret = domain_selector_->SetEndpoints(&info);
  EXPECT_NE(0, ret);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  ret = domain_selector_->Select(&select_info, &endpoint);
  EXPECT_NE(0, ret);

  ret = domain_selector_->Select(nullptr, &endpoint);
  EXPECT_NE(0, ret);

  domain_selector_->AsyncSelect(nullptr).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return MakeReadyFuture<>();
  });

  ret = domain_selector_->SelectBatch(nullptr, nullptr);
  EXPECT_NE(0, ret);

  domain_selector_->AsyncSelectBatch(nullptr).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return MakeReadyFuture<>();
  });
}

TEST_F(SelectorDomainTest, ayncselect_test) {
  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  domain_selector_->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;

  domain_selector_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    TrpcEndpointInfo endpoint = fut.GetValue0();
    if (endpoint.is_ipv6) {
      EXPECT_TRUE(endpoint.host == "::1");
    } else {
      EXPECT_TRUE(endpoint.host == "127.0.0.1");
    }
    EXPECT_TRUE(endpoint.port == 1001);
    return MakeReadyFuture<>();
  });

  domain_selector_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    TrpcEndpointInfo endpoint = fut.GetValue0();
    if (endpoint.is_ipv6) {
      EXPECT_TRUE(endpoint.host == "::1");
    } else {
      EXPECT_TRUE(endpoint.host == "127.0.0.1");
    }
    EXPECT_TRUE(endpoint.port == 1001);
    return MakeReadyFuture<>();
  });

  domain_selector_->AsyncSelectBatch(&select_info).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
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
  domain_selector_->AsyncSelectBatch(&select_info).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
    EXPECT_TRUE(endpoints.size() > 0);
    return trpc::MakeReadyFuture<>();
  });

  select_info.name = "test_service1";
  domain_selector_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return trpc::MakeReadyFuture<>();
  });

  domain_selector_->AsyncSelectBatch(&select_info).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return trpc::MakeReadyFuture<>();
  });
}

TEST_F(SelectorDomainTest, UpdateEndpointInfo) {
  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1, endpoint2;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  domain_selector_->SetEndpoints(&info);
  // Internal tasks perform node updates once in 200ms
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  domain_selector_->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.port == 1001);
}

TEST_F(SelectorDomainTest, exclude_ipv6_test) {
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
}

// Report failure for 10 consecutive times, expecting switch to open state, and not accessed within the
// 'sleep_window_ms' and recovery occurs when the success criteria are met after reaching the half-open state.
TEST_F(SelectorDomainTest, circuitbreak_when_continuous_fail) {
  std::string domain = "www.qq.com";
  std::vector<std::string> addrs;
  trpc::util::GetAddrFromDomain(domain, addrs);
  // sort ip, Duplicate removal
  std::set<std::string> ip_list_set(addrs.begin(), addrs.end());
  if (ip_list_set.size() < 2) {
    // If the number of domain nodes is less than 2, do not execute this test case.
    return;
  }

  // set endpoints of callee
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "www.qq.com";
  endpoint1.port = 1001;
  endpoints_info.push_back(endpoint1);
  domain_selector_->SetEndpoints(&info);

  SelectorInfo select_info;
  select_info.name = "test_service";
  TrpcEndpointInfo endpoint;
  ASSERT_EQ(domain_selector_->Select(&select_info, &endpoint), 0);
  auto error_endpoint_ip = endpoint.host;

  InvokeResult result;
  result.name = "test_service";
  result.framework_result = ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  result.cost_time = 100;
  result.context = MakeRefCounted<ClientContext>();
  ExtendNodeAddr addr;
  addr.addr.ip = endpoint.host;
  addr.addr.port = endpoint.port;
  result.context->SetRequestAddrByNaming(std::move(addr));
  for (uint32_t i = 0; i < circuit_break_config_.continuous_error_threshold; i++) {
    domain_selector_->ReportInvokeResult(&result);
  }

  uint32_t call_times = 100;
  for (uint32_t i = 0; i < call_times; i++) {
    domain_selector_->Select(&select_info, &endpoint);
    ASSERT_NE(endpoint.host, error_endpoint_ip);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(circuit_break_config_.sleep_window_ms));
  // If the number of successful calls meets the requirements, it will be switch from half-open state to close state.
  for (uint32_t i = 0, j = 0; i < circuit_break_config_.request_count_after_half_open && j < call_times; j++) {
    domain_selector_->Select(&select_info, &endpoint);
    result.framework_result = ::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS;
    domain_selector_->ReportInvokeResult(&result);
    if (endpoint.host == error_endpoint_ip) {
      i++;
    }
  }

  // check the probability of the node being called
  int call_count = 0;
  for (uint32_t i = 0; i < call_times; i++) {
    int ret = domain_selector_->Select(&select_info, &endpoint);
    ASSERT_EQ(ret, 0);
    if (endpoint.host == error_endpoint_ip) {
      call_count++;
    }
  }

  float call_count_rate = call_count * 1.0 / call_times;
  ASSERT_TRUE(call_count_rate > 0.2 && call_count_rate < 0.8) << call_count_rate << std::endl;
}

}  // namespace trpc
