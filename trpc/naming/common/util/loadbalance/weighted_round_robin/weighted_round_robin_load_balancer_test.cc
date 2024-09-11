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

#include <memory>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/naming/direct/selector_direct.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/util/loadbalance/weighted_round_robin/weighted_round_robin_load_balancer.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
namespace trpc {

class SWRoundRobinLoadBalanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    PeripheryTaskScheduler::GetInstance()->Init();
    PeripheryTaskScheduler::GetInstance()->Start();

    load_balance_ = MakeRefCounted<SWRoundRobinLoadBalance>();
    test_polling_load_balance_ = MakeRefCounted<SelectorDirect>(load_balance_);
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
  SelectorDirectPtr test_polling_load_balance_;
};
TEST_F(SWRoundRobinLoadBalanceTest, SelectWithWeights) {
  RouterInfo info;
  info.name = "test_service";

  // Initialize endpoints with different weights
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1;
  endpoint1.host = "192.168.1.1";
  endpoint1.port = 10000;
  endpoint1.weight = 10;
  endpoints_info.push_back(endpoint1);

  TrpcEndpointInfo endpoint2;
  endpoint2.host = "192.168.1.2";
  endpoint2.port = 20000;
  endpoint2.weight = 20;
  endpoints_info.push_back(endpoint2);

  TrpcEndpointInfo endpoint3;
  endpoint2.host = "192.168.1.3";
  endpoint2.port = 30000;
  endpoint2.weight = 30;
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
    EXPECT_TRUE(selected_endpoint.host == "192.168.1.1" || selected_endpoint.host == "192.168.1.2"||selected_endpoint.host == "192.168.1.3");
    EXPECT_TRUE(selected_endpoint.port == 10000 || selected_endpoint.port == 20000||selected_endpoint.port == 30000);
    EXPECT_TRUE(selected_endpoint.id != kInvalidEndpointId);
    count_map[selected_endpoint.port]++;
  }

  EXPECT_GT(count_map[10000], 0);
  EXPECT_GT(count_map[20000], 0);
  EXPECT_GT(count_map[30000], 0);
  EXPECT_EQ(count_map.size(),3);

  // Check the proportion of selections
  float weight_ratio1 = 20.0 / 10.0;  
  float weight_ratio2 = 30.0 / 10.0;  
  float weight_ratio3 = 30.0 / 20.0;  
  EXPECT_NEAR(count_map[20000] / float(count_map[10000]), weight_ratio1, 0.1);
  EXPECT_NEAR(count_map[30000] / float(count_map[10000]), weight_ratio2, 0.1);
  EXPECT_NEAR(count_map[30000] / float(count_map[20000]), weight_ratio3, 0.1);
}
}  // namespace trpc