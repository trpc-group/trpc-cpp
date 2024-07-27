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

#include "wrr_load_balance.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/naming/direct/selector_direct.h"

namespace trpc {

TEST(WrrLoadBalance, wrr_load_balance_test) {
  WrrLoadBalance* wrr= new WrrLoadBalance();
  std::vector<TrpcEndpointInfo> endpoints_info;
  TrpcEndpointInfo endpoint1, endpoint2, endpoint3;
  endpoint1.host = "127.000.000.001";
  endpoint1.port = 1001;
  endpoint2.host = "192.168.000.002";
  endpoint2.port = 1002;
  endpoint3.host = "114.514.191.981";
  endpoint3.port = 1003;
  endpoints_info.push_back(endpoint1);
  endpoints_info.push_back(endpoint2);
  endpoints_info.push_back(endpoint3);

  SelectorInfo select_info;
  select_info.name = "test_service";
  select_info.context = nullptr;
  LoadBalanceInfo load_balance_info;
  load_balance_info.info = &select_info;
  load_balance_info.endpoints = &endpoints_info;
  wrr->Update(&load_balance_info);

  // Set weight of each endpoint
  std::vector<int> weights = {1, 2 ,4};
  wrr->SetWeight(weights, &select_info);

  int counts[3] = {0, 0, 0};
  for (int i = 0; i < 14; i++)
  {
    LoadBalanceResult load_balance_result;
    load_balance_result.info = &select_info;
    if (wrr->Next(load_balance_result)) {
      std::string error_str = "Do load balance of " + std::to_string(i) + " failed";
      TRPC_LOG_ERROR(error_str);
    }
    TrpcEndpointInfo endpoint= std::any_cast<TrpcEndpointInfo>(load_balance_result.result);

    std::cout << "host: " << endpoint.host << " port: " << endpoint.port << std::endl;
    if (endpoint.port == 1001) counts[0]++;
    else if (endpoint.port == 1002) counts[1]++;
    else if (endpoint.port == 1003) counts[2]++;
  }
  std::cout << std::endl;
  std::cout << "port:1001 count: " << counts[0] << std::endl;
  std::cout << "port:1002 count: " << counts[1] << std::endl;
  std::cout << "port:1003 count: " << counts[2] << std::endl;
  EXPECT_EQ(2, counts[0]);
  EXPECT_EQ(4, counts[1]);
  EXPECT_EQ(8, counts[2]);
}
}  // namespace trpc
