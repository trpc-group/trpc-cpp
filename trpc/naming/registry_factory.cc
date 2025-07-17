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

#include "trpc/naming/registry_factory.h"

#include "trpc/util/log/logging.h"

namespace trpc {

bool RegistryFactory::Register(const RegistryPtr& registry) {
  TRPC_ASSERT(registry != nullptr);
  registrys_[registry->Name()] = registry;

  return true;
}

RegistryPtr RegistryFactory::Get(const std::string& registry_name) {
  auto iter = registrys_.find(registry_name);
  if (iter != registrys_.end()) {
    return iter->second;
  }
  return nullptr;
}

const std::unordered_map<std::string, RegistryPtr>& RegistryFactory::GetAllPlugins() { return registrys_; }

void RegistryFactory::Clear() { registrys_.clear(); }

}  // namespace trpc
