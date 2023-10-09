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

#include "trpc/naming/load_balance_factory.h"

#include "trpc/util/log/logging.h"

namespace trpc {

bool LoadBalanceFactory::Register(const LoadBalancePtr& load_balancer) {
  TRPC_ASSERT(load_balancer != nullptr);

  load_balances_[load_balancer->Name()] = load_balancer;

  return true;
}

LoadBalancePtr LoadBalanceFactory::Get(const std::string& name) {
  auto iter = load_balances_.find(name);
  if (iter != load_balances_.end()) {
    return iter->second;
  }
  return nullptr;
}

const std::unordered_map<std::string, LoadBalancePtr>& LoadBalanceFactory::GetAllPlugins() { return load_balances_; }

void LoadBalanceFactory::Clear() {
  for (auto& lb : load_balances_) {
    lb.second->Destroy();
  }

  load_balances_.clear();
}

}  // namespace trpc
