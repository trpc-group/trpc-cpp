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

#pragma once

#include <string>
#include <unordered_map>

#include "trpc/auth/auth.h"

namespace trpc {

/// @brief Auth center synchronizer factory class used for registering and obtaining various Auth synchronizer
/// instances.
class AuthCenterFollowerFactory {
 public:
  static AuthCenterFollowerFactory* GetInstance() {
    static AuthCenterFollowerFactory instance;
    return &instance;
  }

  /// @brief Registers an auth center follower instance.
  /// @note: The registration method is currently not thread-safe, and registration needs to be completed in advance
  /// during program initialization.
  bool Register(const AuthCenterFollowerPtr &auth_center_follower);

  /// @brief Gets an auth center follower instance by |auth_name|.
  AuthCenterFollowerPtr Get(const std::string &auth_name);

  /// @brief Get all auth center follower plugins
  const std::unordered_map<std::string, AuthCenterFollowerPtr>& GetAllAuthCenterFollowerPlugins();

  /// @brief Clears the auth instances.
  /// @note Used internally by the framework.
  void Clear();

 private:
  AuthCenterFollowerFactory() = default;

  AuthCenterFollowerFactory(const AuthCenterFollowerFactory&) = delete;
  AuthCenterFollowerFactory& operator=(const AuthCenterFollowerFactory&) = delete;

 private:
  std::unordered_map<std::string, AuthCenterFollowerPtr> auth_center_followers_;
};

}  // namespace trpc
