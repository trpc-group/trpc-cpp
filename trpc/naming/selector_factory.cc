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

#include "trpc/naming/selector_factory.h"

#include "trpc/util/log/logging.h"

namespace trpc {

bool SelectorFactory::Register(const SelectorPtr& selector) {
  TRPC_ASSERT(selector != nullptr);
  selectors_[selector->Name()] = selector;

  return true;
}

SelectorPtr SelectorFactory::Get(const std::string& selector_name) {
  auto it = selectors_.find(selector_name);
  if (it != selectors_.end()) {
    return it->second;
  }

  return nullptr;
}

const std::unordered_map<std::string, SelectorPtr>& SelectorFactory::GetAllPlugins() { return selectors_; }

void SelectorFactory::Clear() { selectors_.clear(); }

}  // namespace trpc
