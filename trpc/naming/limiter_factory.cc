//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/naming/limiter_factory.h"

#include "trpc/util/log/logging.h"

namespace trpc {

bool LimiterFactory::Register(const LimiterPtr& limiter) {
  TRPC_ASSERT(limiter != nullptr);

  limiters_[limiter->Name()] = limiter;

  return true;
}

LimiterPtr LimiterFactory::Get(const std::string& limiter_name) {
  auto it = limiters_.find(limiter_name);
  if (it != limiters_.end()) {
    return it->second;
  }

  return nullptr;
}

const std::unordered_map<std::string, LimiterPtr>& LimiterFactory::GetAllPlugins() { return limiters_; }

void LimiterFactory::Clear() { limiters_.clear(); }

}  // namespace trpc
