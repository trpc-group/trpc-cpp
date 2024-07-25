//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <string>
#include <vector>

#include "trpc/common/config/loadbalance_naming_conf.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/common/util/hash/hash_func.h"

namespace trpc {

  
std::string GenerateKeysAsString(const SelectorInfo* info, std::vector<uint32_t>& indexs);

bool CheckLoadBalanceSelectorConfig(naming::LoadBalanceSelectorConfig& loadbalance_config_);

bool CheckLoadbalanceInfoDiff(const std::vector<TrpcEndpointInfo>& orig_endpoints,const std::vector<TrpcEndpointInfo>* new_endpoints);
}  // namespace trpc