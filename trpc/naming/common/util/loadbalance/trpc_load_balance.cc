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


namespace trpc::loadbalance {

bool Init() {

  int res=0;
  
  // Registers default loadbalance plugins which provided by the framework.
  LoadBalancePtr polling_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kPollingLoadBalance);
  if (polling_load_balance == nullptr) {
    polling_load_balance = MakeRefCounted<PollingLoadBalance>();
    res+=polling_load_balance->Init();
    LoadBalanceFactory::GetInstance()->Register(polling_load_balance);
  }

  LoadBalancePtr consistenthash_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kConsistentHashLoadBalance);
  if (consistenthash_load_balance == nullptr) {
    consistenthash_load_balance = MakeRefCounted<ConsistentHashLoadBalance>();
    res+=consistenthash_load_balance->Init();
    LoadBalanceFactory::GetInstance()->Register(consistenthash_load_balance);
  }

  LoadBalancePtr modulohash_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kModuloHashLoadBalance);
  if (modulohash_load_balance == nullptr) {
    modulohash_load_balance = MakeRefCounted<ModuloHashLoadBalance>();
    res+=modulohash_load_balance->Init();
    LoadBalanceFactory::GetInstance()->Register(modulohash_load_balance);
  }

  return res==0;
}

void Stop() {}

void Destroy() { LoadBalanceFactory::GetInstance()->Clear(); }

}  // namespace trpc::loadbalance
