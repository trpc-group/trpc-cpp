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

#include "trpc/naming/pulling_test/selector_load_balance.h"
#include <memory>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/loadbalance/weighted_round_robin/weighted_round_robin_load_balancer.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
namespace trpc {

class SmoothWeightedPollingLoadBalanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PeripheryTaskScheduler::GetInstance()->Init();
    PeripheryTaskScheduler::GetInstance()->Start();

    load_balance_ = MakeRefCounted<SmoothWeightedPollingLoadBalance>();
    test_polling_load_balance_ = MakeRefCounted<TestPollingLoadBalance>(load_balance_);
    test_polling_load_balance_->Init();
    test_polling_load_balance_->Start();
  }

  void TearDown() override {
    test_polling_load_balance_->Stop();
    test_polling_load_balance_->Destroy();

    PeripheryTaskScheduler::GetInstance()->Stop();
    PeripheryTaskScheduler::GetInstance()->Join();
  }

  LoadBalancePtr load_balance_;
  TestPollingLoadBalancePtr test_polling_load_balance_;
};
TEST_F(SmoothWeightedPollingLoadBalanceTest, TestSingleSelectionAccuracy) {
  RouterInfo info;
  info.name = "test_service";

  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "192.168.1.1";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = context;
  TrpcEndpointInfo endpoint;

  test_polling_load_balance_->Select(&select_info, &endpoint);
  EXPECT_TRUE(endpoint.host == "192.168.1.1");
  EXPECT_TRUE(endpoint.port == 1001);
  EXPECT_TRUE(endpoint.id != kInvalidEndpointId);

  test_polling_load_balance_->AsyncSelect(&select_info).Then([](Future<trpc::TrpcEndpointInfo>&& fut) {
    TrpcEndpointInfo endpoint = fut.GetValue0();
    EXPECT_TRUE(endpoint.host == "192.168.1.1");
    EXPECT_TRUE(endpoint.port == 1001);
    EXPECT_TRUE(endpoint.id != kInvalidEndpointId);
    return MakeReadyFuture<>();
  });

  std::vector<TrpcEndpointInfo> endpoints;
  test_polling_load_balance_->SelectBatch(&select_info, &endpoints);
  for (const auto& ref : endpoints) {
    EXPECT_TRUE(ref.host == "192.168.1.1");
    EXPECT_TRUE(ref.port == 1001);
    EXPECT_TRUE(ref.id != kInvalidEndpointId);
  }

  endpoints.clear();
  select_info.policy = SelectorPolicy::MULTIPLE;
  int ret = test_polling_load_balance_->SelectBatch(&select_info, &endpoints);
  EXPECT_EQ(0, ret);
  EXPECT_TRUE(endpoints.size() > 0);

  select_info.name = "test_service1";
  ret = test_polling_load_balance_->Select(&select_info, &endpoint);
  EXPECT_NE(0, ret);

  test_polling_load_balance_->SelectBatch(&select_info, &endpoints);
  EXPECT_NE(0, ret);
}

TEST_F(SmoothWeightedPollingLoadBalanceTest, SelectWithWeights) {
  RouterInfo info;
  info.name = "test_service";

  // Initialize endpoints with different weights
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "192.168.1.1";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "192.168.1.2";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform multiple selections and verify the distribution
  std::unordered_map<int, int> count_map;
  for (int i = 0; i < 1000; ++i) {
    SelectorInfo select_info;
    select_info.name = "test_service";
    TrpcEndpointInfo selected_endpoint;
    test_polling_load_balance_->Select(&select_info, &selected_endpoint);
    EXPECT_TRUE(selected_endpoint.host == "192.168.1.1" || selected_endpoint.host == "192.168.1.2");
    EXPECT_TRUE(selected_endpoint.port == 1001 || selected_endpoint.port == 1002);
    EXPECT_TRUE(selected_endpoint.id != kInvalidEndpointId);
    count_map[selected_endpoint.port]++;
  }

  // Verify the results
  EXPECT_GT(count_map[1001], 0);
  EXPECT_GT(count_map[1002], 0);
  EXPECT_EQ(count_map.size(), 2);

  // Check the proportion of selections
  float weight_ratio = 20.0 / 10.0;  // endpoint2 should be selected twice as often as endpoint1
  EXPECT_NEAR(count_map[1002] / float(count_map[1001]), weight_ratio, 0.1);
}

TEST_F(SmoothWeightedPollingLoadBalanceTest, AsyncSelectWithWeights) {
  RouterInfo info;
  info.name = "test_service";

  // Initialize endpoints with different weights
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "192.168.1.1";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "192.168.1.2";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform asynchronous selections and verify the distribution
  std::unordered_map<int, int> count_map;
  std::vector<Future<TrpcEndpointInfo>> futures;

  for (int i = 0; i < 1000; ++i) {
    SelectorInfo select_info;
    select_info.name = "test_service";
    futures.push_back(test_polling_load_balance_->AsyncSelect(&select_info));
  }

  for (auto& future : futures) {
    auto selected_endpoint = future.GetValue0();
    EXPECT_TRUE(selected_endpoint.host == "192.168.1.1" || selected_endpoint.host == "192.168.1.2");
    EXPECT_TRUE(selected_endpoint.port == 1001 || selected_endpoint.port == 1002);
    EXPECT_TRUE(selected_endpoint.id != kInvalidEndpointId);
    count_map[selected_endpoint.port]++;
  }

  // Verify the results
  EXPECT_GT(count_map[1001], 0);
  EXPECT_GT(count_map[1002], 0);

  // Check the proportion of selections
  float weight_ratio = 20.0 / 10.0;  // endpoint2 should be selected twice as often as endpoint1
  EXPECT_NEAR(count_map[1002] / float(count_map[1001]), weight_ratio, 0.1);
}

TEST_F(SmoothWeightedPollingLoadBalanceTest, SelectBatchWithWeights) {
  RouterInfo info;
  info.name = "test_service";
  // Initialize endpoints with different weights
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "192.168.1.1";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "192.168.1.2";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform batch selections
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.policy = SelectorPolicy::MULTIPLE;
  select_info.select_num = 1000;  // number of endpoints to select

  std::vector<TrpcEndpointInfo> selected_endpoints;
  test_polling_load_balance_->SelectBatch(&select_info, &selected_endpoints);

  // Verify the results
  EXPECT_GE(selected_endpoints.size(), 1);

  std::unordered_map<int, int> count_map;
  for (const auto& endpoint : selected_endpoints) {
    EXPECT_TRUE(endpoint.host == "192.168.1.1" || endpoint.host == "192.168.1.2");
    EXPECT_TRUE(endpoint.port == 1001 || endpoint.port == 1002);
    EXPECT_TRUE(endpoint.id != kInvalidEndpointId);
    count_map[endpoint.port]++;
  }

  // Verify the proportion of selections
  float weight_ratio = 20.0 / 10.0;  // endpoint2 should be selected twice as often as endpoint1
  EXPECT_NEAR(count_map[1002] / float(count_map[1001]), weight_ratio, 0.1);
}

TEST_F(SmoothWeightedPollingLoadBalanceTest, AsyncSelectBatchWithWeights) {
  RouterInfo info;
  info.name = "test_service";

  // Initialize endpoints with different weights
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "192.168.1.1";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "192.168.1.2";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform asynchronous batch selections
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.policy = SelectorPolicy::MULTIPLE;
  select_info.select_num = 1000;  // number of endpoints to select

  auto future = test_polling_load_balance_->AsyncSelectBatch(&select_info);
  future
      .Then([](Future<std::vector<TrpcEndpointInfo>>&& fut) {
        auto selected_endpoints = fut.GetValue0();
        std::unordered_map<int, int> count_map;

        for (const auto& endpoint : selected_endpoints) {
          EXPECT_TRUE(endpoint.host == "192.168.1.1" || endpoint.host == "192.168.1.2");
          EXPECT_TRUE(endpoint.port == 1001 || endpoint.port == 1002);
          EXPECT_TRUE(endpoint.id != kInvalidEndpointId);
          count_map[endpoint.port]++;
        }

        // Verify the results
        EXPECT_GE(selected_endpoints.size(), 1);

        // Verify the proportion of selections
        float weight_ratio = 20.0 / 10.0;  // endpoint2 should be selected twice as often as endpoint1
        EXPECT_NEAR(count_map[1002] / float(count_map[1001]), weight_ratio, 0.1);
        return MakeReadyFuture<>();
      })
      .GetValue();  // Wait for completion of asynchronous future
}

TEST_F(SmoothWeightedPollingLoadBalanceTest, DynamicEndpointAddRemove) {
  RouterInfo info;
  info.name = "test_service";

  // Add initial endpoints
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "192.168.1.1";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform initial selection
  SelectorInfo select_info;
  select_info.name = "test_service";
  TrpcEndpointInfo selected_endpoint;
  test_polling_load_balance_->Select(&select_info, &selected_endpoint);
  EXPECT_EQ(selected_endpoint.port, 1001);

  // Add another endpoint
  TrpcEndpointInfo endpoint2;
  endpoint2.host = "192.168.1.2";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);
  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform selection after adding endpoint
  test_polling_load_balance_->Select(&select_info, &selected_endpoint);
  EXPECT_TRUE(selected_endpoint.port == 1001 || selected_endpoint.port == 1002);

  // Remove an endpoint
  endpoints_info.erase(std::remove_if(endpoints_info.begin(), endpoints_info.end(),
                                      [](const TrpcEndpointInfo& ep) { return ep.port == 1001; }),
                       endpoints_info.end());
  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform selection after removing endpoint

  std::vector<TrpcEndpointInfo> selected_endpoints;
  select_info.select_num = 100;
  test_polling_load_balance_->SelectBatch(&select_info, &selected_endpoints);
  for (const auto& endpoint : selected_endpoints) {
    EXPECT_EQ(endpoint.port, 1002);
  }
}
}  // namespace trpc