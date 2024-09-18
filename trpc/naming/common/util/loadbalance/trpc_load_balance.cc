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
#include "trpc/naming/common/util/loadbalance/weighted_round_robin/weighted_round_robin_load_balancer.h"
#include "trpc/naming/load_balance_factory.h"

namespace trpc::loadbalance {

bool Init() {
  LoadBalancePtr swround_robin_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kSWRoundRobinLoadBalance);
  if (swround_robin_load_balance == nullptr) {
    // Register the default load balancer
    swround_robin_load_balance = MakeRefCounted<SWRoundRobinLoadBalance>();
    LoadBalanceFactory::GetInstance()->Register(swround_robin_load_balance);
  }
  return true;
}

void Stop() {}

void Destroy() { LoadBalanceFactory::GetInstance()->Clear(); }

}  // namespace trpc::loadbalance