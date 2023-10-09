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

#include "trpc/auth/auth_center_follower_factory.h"

namespace trpc {

bool AuthCenterFollowerFactory::Register(const AuthCenterFollowerPtr &auth_center_follower) {
  auto name = auth_center_follower->Name();
  auto it = auth_center_followers_.find(name);
  if (it == auth_center_followers_.end()) {
    auth_center_followers_[name] = auth_center_follower;
    return true;
  }
  return false;
}

AuthCenterFollowerPtr AuthCenterFollowerFactory::Get(const std::string &auth_name) {
  AuthCenterFollowerPtr auth_center_follower = nullptr;
  auto found = auth_center_followers_.find(auth_name);
  if (auth_center_followers_.end() != found) auth_center_follower = found->second;
  return auth_center_follower;
}

const std::unordered_map<std::string, AuthCenterFollowerPtr>& AuthCenterFollowerFactory::GetAllAuthCenterFollowerPlugins() {  // NOLINT
  return auth_center_followers_;
}

void AuthCenterFollowerFactory::Clear() {
  for (auto& item : auth_center_followers_) {
    item.second->Stop();
  }
  auth_center_followers_.clear();
}

}  // namespace trpc
