//#include "test_weight_polling_load_balance.h"
#include "trpc/naming/load_balance.h"
#include "weight_polling_load_balance.h"
#include <iostream>

void TestWeightPollingLoadBalance() {
  using namespace trpc;

  WeightPollingLoadBalance lb;

  // 定义一些测试节点
  std::vector<TrpcEndpointInfo> endpoints = {
    {
        "A",  // host   为了方便看
        4,           // port   为了方便看，写成权重
        1,              // status (表示健康状态的整数)
        false,          // is_ipv6 (表示是否为IPv6地址)
        4,              // weight (权重)
        1001,           // id (节点的唯一ID)
        {{"region", "us-east"}, {"version", "v1"}}  // meta (扩展信息字段)
    },
    {
        "B",
        2,
        1,
        false,
        2,
        1002,
        {{"region", "us-west"}, {"version", "v1"}}
    },
    {
        "C",
        1,
        1,
        false,
        1,
        1003,
        {{"region", "eu-central"}, {"version", "v2"}}
    }
};

  LoadBalanceInfo info;
  SelectorInfo selector_info;
  selector_info.name = "test_service";
  info.info = &selector_info;
  info.endpoints = &endpoints;

  lb.Update(&info);

  // 模拟请求并测试负载均衡结果
  const int request_count = 14;  // 总请求数
  std::unordered_map<std::string, int> request_distribution;

  for (int i = 0; i < request_count; ++i) {
    LoadBalanceResult result;
    result.info = &selector_info;
    lb.Next(result);
     // 将 std::any 转换为 TrpcEndpointInfo
    TrpcEndpointInfo endpoint = std::any_cast<TrpcEndpointInfo>(result.result);
    std::string endpoint_key = endpoint.host + ":" + std::to_string(endpoint.port);
    
    std::cout << "Request #" << (i + 1) << ": Endpoint: " << endpoint_key << ": ID: " << endpoint.id<< std::endl;

     ++request_distribution[endpoint_key];
  }

    std::cout << "Request distribution after " << request_count << " requests:" << std::endl;
    for (const auto& entry : request_distribution) {
        std::cout << "Endpoint: " << entry.first << ", Requests: " << entry.second << std::endl;
    }
}

int main() {
  TestWeightPollingLoadBalance();
  return 0;
}
