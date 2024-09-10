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

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
namespace trpc::naming {
struct SWRoundrobinLoadBalanceConfig {
  // Mapping from service_name to a map of address (IP:Port) to weight
  std::unordered_map<std::string, std::vector<uint32_t>> services_weight;
  void Display() const;  // Function to display the contents of the struct
};

}  // namespace trpc::naming
