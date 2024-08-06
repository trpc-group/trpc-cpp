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

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>
#include "trpc/naming/pulling_test/selector_load_balance.h"

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

TEST(SmoothWeightedPollingLoadBalanceTest, SelectWithWeights) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  LoadBalancePtr polling = MakeRefCounted<SmoothWeightedPollingLoadBalance>();
  TestPollingLoadBalancePtr ptr = MakeRefCounted<TestPollingLoadBalance>(polling);
  ptr->Init();
  ptr->Start();
  EXPECT_TRUE(ptr->Version() != "");

  RouterInfo info;
  info.name = "test_service";

  // Initialize endpoints with different weights
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "localhost";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  info.info = endpoints_info;
  ptr->SetEndpoints(&info);

  // Perform multiple selections and verify the distribution
  std::unordered_map<int, int> count_map;
  for (int i = 0; i < 1000; ++i) {
    SelectorInfo select_info;
    select_info.name = "test_service";
    TrpcEndpointInfo selected_endpoint;
    ptr->Select(&select_info, &selected_endpoint);
    count_map[selected_endpoint.port]++;
  }

  // Verify the results
  EXPECT_GT(count_map[1001], 0);
  EXPECT_GT(count_map[1002], 0);

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
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "localhost";
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
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "localhost";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform batch selections
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.policy = SelectorPolicy::MULTIPLE;
  select_info.select_num = 10;  // number of endpoints to select

  std::vector<TrpcEndpointInfo> selected_endpoints;
  test_polling_load_balance_->SelectBatch(&select_info, &selected_endpoints);

  // Verify the results
  EXPECT_GE(selected_endpoints.size(), 1);

  std::unordered_map<int, int> count_map;
  for (const auto& endpoint : selected_endpoints) {
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
  endpoint1.host = "localhost";
  endpoint1.port = 1001;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "localhost";
  endpoint2.port = 1002;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  info.info = endpoints_info;
  test_polling_load_balance_->SetEndpoints(&info);

  // Perform asynchronous batch selections
  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.policy = SelectorPolicy::MULTIPLE;
  select_info.select_num = 10;  // number of endpoints to select

  auto future = test_polling_load_balance_->AsyncSelectBatch(&select_info);
  future
      .Then([](Future<std::vector<TrpcEndpointInfo>>&& fut) {
        auto selected_endpoints = fut.GetValue0();
        std::unordered_map<int, int> count_map;

        for (const auto& endpoint : selected_endpoints) {
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

}  // namespace trpc
