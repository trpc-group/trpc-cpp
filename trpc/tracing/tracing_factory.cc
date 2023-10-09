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

#include "trpc/tracing/tracing_factory.h"

#include "trpc/util/log/logging.h"

namespace trpc {

bool TracingFactory::Register(const TracingPtr& tracing) {
  TRPC_ASSERT(tracing != nullptr);

  tracing_plugins_[tracing->Name()] = tracing;

  return true;
}

TracingPtr TracingFactory::Get(const std::string& name) {
  auto iter = tracing_plugins_.find(name);
  if (iter != tracing_plugins_.end()) {
    return iter->second;
  }
  return nullptr;
}

const std::unordered_map<std::string, TracingPtr>& TracingFactory::GetAllPlugins() { return tracing_plugins_; }

void TracingFactory::Clear() { tracing_plugins_.clear(); }

}  // namespace trpc
