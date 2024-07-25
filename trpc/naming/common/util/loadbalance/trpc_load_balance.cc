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

#include "trpc/naming/common/util/loadbalance/trpc_load_balance.h"

#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/common/util/loadbalance/hash/consistenthash_load_balance.h"
#include "trpc/naming/common/util/loadbalance/hash/modulohash_load_balance.h"
#include "trpc/naming/common/util/loadbalance/weighted_round_robin/weighted_round_robin_load_balancer.h"


namespace trpc::loadbalance {

bool Init() {
  LoadBalancePtr swround_robin_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kSWRoundRobinLoadBalance);
  if (swround_robin_load_balance == nullptr) {
    // Register the default load balancer
    swround_robin_load_balance = MakeRefCounted<SWRoundRobinLoadBalance>();
    LoadBalanceFactory::GetInstance()->Register(swround_robin_load_balance);
  }
  bool res = true;

  // Registers default loadbalance plugins which provided by the framework.
  LoadBalancePtr polling_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kPollingLoadBalance);
  if (polling_load_balance == nullptr) {
    polling_load_balance = MakeRefCounted<PollingLoadBalance>();
    int ret = polling_load_balance->Init();

    if (!ret) {
      LoadBalanceFactory::GetInstance()->Register(polling_load_balance);
    } else {
      res = false;
    }
  }

  LoadBalancePtr consistenthash_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kConsistentHashLoadBalance);
  if (consistenthash_load_balance == nullptr) {
    consistenthash_load_balance = MakeRefCounted<ConsistentHashLoadBalance>();
    int ret = consistenthash_load_balance->Init();
    if (!ret) {
      LoadBalanceFactory::GetInstance()->Register(consistenthash_load_balance);
    } else {
      res = false;
    }
  }

  LoadBalancePtr modulohash_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kModuloHashLoadBalance);
  if (modulohash_load_balance == nullptr) {
    modulohash_load_balance = MakeRefCounted<ModuloHashLoadBalance>();
    int ret = modulohash_load_balance->Init();
    if (!ret) {
      LoadBalanceFactory::GetInstance()->Register(modulohash_load_balance);
    } else {
      res = false;
    }
  }

  return res;
}

void Stop() {}

void Destroy() { LoadBalanceFactory::GetInstance()->Clear(); }

}  // namespace trpc::loadbalance
