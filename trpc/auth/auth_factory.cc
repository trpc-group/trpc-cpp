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

#include "trpc/auth/auth_factory.h"

namespace trpc {

bool AuthFactory::Register(const AuthPtr& auth) {
  auto name = auth->Name();
  auto it = auths_.find(name);
  if (it == auths_.end()) {
    auths_[name] = auth;
    return true;
  }
  return false;
}

AuthPtr AuthFactory::Get(const std::string& codec_name) {
  auto found = auths_.find(codec_name);
  if (auths_.end() != found) return found->second;
  return nullptr;
}

const std::unordered_map<std::string, AuthPtr>& AuthFactory::GetAllAuthPlugins() {
  return auths_;
}

void AuthFactory::Clear() { auths_.clear(); }

}  // namespace trpc
