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
#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/circuit_break/circuit_breaker_creator_factory.h"
#include "trpc/naming/common/util/circuit_break/default_circuit_breaker.h"
#include "trpc/naming/common/util/circuit_break/default_circuit_breaker_config.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/direct/direct_selector_conf_parser.h"

namespace trpc {

class SelectorDirectTest : public ::testing::Test {
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
  }

  static void TearDownTestCase() {}

  void SetUp() override {
    int ret = TrpcConfig::GetInstance()->Init("./trpc/naming/testing/direct_selector_test.yaml");
    ASSERT_TRUE(ret == 0);
    naming::DirectSelectorConfig config;
    trpc::TrpcConfig::GetInstance()->GetPluginConfig("selector", "direct", config);
    YAML::convert<naming::DefaultCircuitBreakerConfig>::decode(config.circuit_break_config.plugin_config,
                                                               circuit_break_config_);

    LoadBalancePtr polling = MakeRefCounted<PollingLoadBalance>();
    direct_selector_ = std::make_shared<SelectorDirect>(polling);
    direct_selector_->Init();
    direct_selector_->Start();
    EXPECT_TRUE(direct_selector_->Version() != "");

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
    direct_selector_->SetEndpoints(&info);
  }

  void TearDown() override {
    direct_selector_->Stop();
    direct_selector_->Destroy();
  }

 protected:
  std::shared_ptr<SelectorDirect> direct_selector_;
  naming::DefaultCircuitBreakerConfig circuit_break_config_;
};

TEST_F(SelectorDirectTest, select_test) {
  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  direct_selector_->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.host == "127.0.0.1") << (endpoint.host);
  EXPECT_TRUE(endpoint.port == 1001);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  direct_selector_->Select(&select_info, &endpoint);
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
  ExtendNodeAddr addr;
  addr.addr.ip = "127.0.0.1";
  addr.addr.port = 1001;
  result.context->SetRequestAddrByNaming(std::move(addr));

  int ret = direct_selector_->ReportInvokeResult(&result);
  EXPECT_EQ(0, ret);

  EXPECT_TRUE(direct_selector_->ReportInvokeResult(nullptr) != 0);

  std::vector<TrpcEndpointInfo> endpoints;
  direct_selector_->SelectBatch(&select_info, &endpoints);
  EXPECT_EQ(2, endpoints.size());

  // test select multiple
  endpoints.clear();
  select_info.policy = SelectorPolicy::MULTIPLE;
  ret = direct_selector_->SelectBatch(&select_info, &endpoints);
  EXPECT_EQ(0, ret);

  select_info.name = "test_service1";
  ret = direct_selector_->Select(&select_info, &endpoint);
  EXPECT_NE(0, ret);

  endpoints.clear();
  ret = direct_selector_->SelectBatch(&select_info, &endpoints);
  EXPECT_NE(0, ret);
}

TEST_F(SelectorDirectTest, select_exception_test) {
  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  int ret = direct_selector_->SelectBatch(nullptr, nullptr);
  EXPECT_NE(0, ret);

  direct_selector_->AsyncSelectBatch(nullptr).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return MakeReadyFuture<>();
  });
}

TEST_F(SelectorDirectTest, asyncselect_test) {
  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  direct_selector_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    TrpcEndpointInfo endpoint = fut.GetValue0();
    EXPECT_TRUE(endpoint.host == "127.0.0.1");
    EXPECT_TRUE(endpoint.port == 1001);
    return trpc::MakeReadyFuture<>();
  });

  direct_selector_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    TrpcEndpointInfo endpoint = fut.GetValue0();
    EXPECT_TRUE(endpoint.host == "127.0.0.1");
    EXPECT_TRUE(endpoint.port == 1002);
    return trpc::MakeReadyFuture<>();
  });

  direct_selector_->AsyncSelectBatch(&select_info)
      .Then([this](trpc::Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
        EXPECT_TRUE(fut.IsReady());
        std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
        EXPECT_EQ(2, endpoints.size());
        return trpc::MakeReadyFuture<>();
      });

  // test select multiple
  select_info.policy = SelectorPolicy::MULTIPLE;
  direct_selector_->AsyncSelectBatch(&select_info)
      .Then([this](trpc::Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
        EXPECT_TRUE(fut.IsReady());
        std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
        EXPECT_EQ(2, endpoints.size());
        return trpc::MakeReadyFuture<>();
      });

  select_info.name = "test_service1";
  direct_selector_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return trpc::MakeReadyFuture<>();
  });

  direct_selector_->AsyncSelectBatch(&select_info).Then([](Future<std::vector<trpc::TrpcEndpointInfo>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return trpc::MakeReadyFuture<>();
  });
}

TEST_F(SelectorDirectTest, endpoint_unique_id_test) {
  SelectorInfo select_info1;
  select_info1.name = "test_service";
  auto context1 = trpc::MakeRefCounted<trpc::ClientContext>();
  select_info1.context = context1;
  select_info1.policy = SelectorPolicy::ALL;
  std::vector<TrpcEndpointInfo> result1;
  direct_selector_->SelectBatch(&select_info1, &result1);

  // Reset endpoints info: put endpoint2 after
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "127.0.0.1";
  endpoint1.port = 1001;
  TrpcEndpointInfo endpoint3;
  endpoint3.host = "127.0.0.1";
  endpoint3.port = 1003;
  TrpcEndpointInfo endpoint2;
  endpoint2.host = "127.0.0.1";
  endpoint2.port = 1002;
  RouterInfo info;
  info.name = "test_service";
  std::vector<TrpcEndpointInfo>& endpoints_info = info.info;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint3);
  endpoints_info.push_back(endpoint2);
  direct_selector_->SetEndpoints(&info);

  SelectorInfo select_info2;
  select_info2.name = "test_service";
  auto context2 = trpc::MakeRefCounted<trpc::ClientContext>();
  select_info2.context = context2;
  select_info2.policy = SelectorPolicy::ALL;
  std::vector<TrpcEndpointInfo> result2;
  direct_selector_->SelectBatch(&select_info2, &result2);
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

// Report failure for 10 consecutive times, expecting switch to open state, and not accessed within the
// 'sleep_window_ms' and recovery occurs when the success criteria are met after reaching the half-open state.
TEST_F(SelectorDirectTest, circuitbreak_when_continuous_fail) {
  InvokeResult result;
  result.name = "test_service";
  result.framework_result = ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  result.cost_time = 100;
  result.context = MakeRefCounted<ClientContext>();
  ExtendNodeAddr addr;
  addr.addr.ip = "127.0.0.1";
  addr.addr.port = 1001;
  result.context->SetRequestAddrByNaming(std::move(addr));
  for (uint32_t i = 0; i < circuit_break_config_.continuous_error_threshold; i++) {
    direct_selector_->ReportInvokeResult(&result);
  }

  uint32_t call_times = 100;
  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;
  for (uint32_t i = 0; i < call_times; i++) {
    direct_selector_->Select(&select_info, &endpoint);
    ASSERT_NE(endpoint.port, 1001);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(circuit_break_config_.sleep_window_ms));
  // After the node switch to half-open state, if the number of successful calls meets the requirements, it will be
  // restored.
  for (uint32_t i = 0, j = 0; i < circuit_break_config_.request_count_after_half_open && j < call_times; j++) {
    direct_selector_->Select(&select_info, &endpoint);
    ExtendNodeAddr addr;
    addr.addr.ip = "127.0.0.1";
    addr.addr.port = endpoint.port;
    result.context->SetRequestAddrByNaming(std::move(addr));
    result.framework_result = ::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS;
    direct_selector_->ReportInvokeResult(&result);
    if (endpoint.port == 1001) {
      i++;
    }
  }

  // check the probability of the node being called
  int call_count = 0;
  for (uint32_t i = 0; i < call_times; i++) {
    int ret = direct_selector_->Select(&select_info, &endpoint);
    ASSERT_EQ(ret, 0);
    if (endpoint.port == 1001) {
      call_count++;
    }
  }

  float call_count_rate = call_count * 1.0 / call_times;
  ASSERT_TRUE(call_count_rate > 0.2 && call_count_rate < 0.8) << call_count_rate << std::endl;
}

// When the error rate exceeds the threshold within the statistical period, expecting switch to open state, and not accessed within the
// 'sleep_window_ms' and re-enter open state when the success criteria aren't met after reaching the half-open state.
TEST_F(SelectorDirectTest, circuitbreak_when_exceed_errorrate) {
  InvokeResult result;
  result.name = "test_service";
  result.cost_time = 100;
  result.context = MakeRefCounted<ClientContext>();
  ExtendNodeAddr addr;
  addr.addr.ip = "127.0.0.1";
  addr.addr.port = 1001;
  result.context->SetRequestAddrByNaming(std::move(addr));
  auto interval = circuit_break_config_.stat_window_ms / circuit_break_config_.request_volume_threshold;
  for (uint32_t i = 0; i <= circuit_break_config_.request_volume_threshold; i++) {
    result.framework_result =
        (i % 2 == 0 ? ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR : ::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS);
    direct_selector_->ReportInvokeResult(&result);

    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(interval));
  uint32_t call_times = 100;
  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;
  for (uint32_t i = 0; i < call_times; i++) {
    direct_selector_->Select(&select_info, &endpoint);
    ASSERT_NE(endpoint.port, 1001);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(circuit_break_config_.sleep_window_ms));
  // After the node is switched from open state to half-open state, if the number of successful calls does not meet the
  // requirements, it will re-enter the open state.
  for (uint32_t i = 0, j = 0; i < circuit_break_config_.request_count_after_half_open && j < call_times; j++) {
    direct_selector_->Select(&select_info, &endpoint);
    ExtendNodeAddr addr;
    addr.addr.ip = "127.0.0.1";
    addr.addr.port = endpoint.port;
    result.context->SetRequestAddrByNaming(std::move(addr));
    result.framework_result =
        (j % 2 == 0 ? ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR : ::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS);
    direct_selector_->ReportInvokeResult(&result);
    if (endpoint.port == 1001) {
      i++;
    }
  }

  // check the probability of the node being called
  for (uint32_t i = 0; i < call_times; i++) {
    direct_selector_->Select(&select_info, &endpoint);
    ASSERT_NE(endpoint.port, 1001);
  }
}

// Recover all when all nodes are circuitbreak open state
TEST_F(SelectorDirectTest, all_endpoints_circuitbreak) {
  InvokeResult result;
  result.name = "test_service";
  result.framework_result = ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  result.cost_time = 100;
  result.context = MakeRefCounted<ClientContext>();
  for (uint32_t i = 0; i < circuit_break_config_.continuous_error_threshold; i++) {
    ExtendNodeAddr addr1;
    addr1.addr.ip = "127.0.0.1";
    addr1.addr.port = 1001;
    result.context->SetRequestAddrByNaming(std::move(addr1));
    direct_selector_->ReportInvokeResult(&result);

    ExtendNodeAddr addr2;
    addr2.addr.ip = "127.0.0.1";
    addr2.addr.port = 1002;
    result.context->SetRequestAddrByNaming(std::move(addr2));
    direct_selector_->ReportInvokeResult(&result);
  }

  // check the probability of the node being called
  uint32_t call_times = 100;
  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;
  uint32_t call_port1_count = 0;
  uint32_t call_port2_count = 0;
  for (uint32_t i = 0; i < call_times; i++) {
    int ret = direct_selector_->Select(&select_info, &endpoint);
    ASSERT_EQ(ret, 0);
    if (endpoint.port == 1001) {
      call_port1_count++;
    } else if (endpoint.port == 1002) {
      call_port2_count++;
    }
  }

  ASSERT_TRUE(call_port1_count + call_port2_count == call_times);
  ASSERT_NEAR(call_port1_count, call_port2_count, call_times * 0.1);
}

TEST_F(SelectorDirectTest, select_multi_with_circuitbreak) {
  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  select_info.policy = SelectorPolicy::MULTIPLE;
  select_info.select_num = 2;
  std::vector<TrpcEndpointInfo> endpoints;
  direct_selector_->SelectBatch(&select_info, &endpoints);
  ASSERT_TRUE(endpoints.size() == static_cast<std::size_t>(select_info.select_num));

  InvokeResult result;
  result.name = "test_service";
  result.framework_result = ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  result.cost_time = 100;
  result.context = MakeRefCounted<ClientContext>();
  for (uint32_t i = 0; i < circuit_break_config_.continuous_error_threshold; i++) {
    ExtendNodeAddr addr;
    addr.addr.ip = "127.0.0.1";
    addr.addr.port = 1001;
    result.context->SetRequestAddrByNaming(std::move(addr));
    direct_selector_->ReportInvokeResult(&result);
  }

  endpoints.clear();
  direct_selector_->SelectBatch(&select_info, &endpoints);
  ASSERT_TRUE(endpoints.size() == 1);
}

}  // namespace trpc
