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

/// @brief Auth factory class used for registering and obtaining various Auth instances.
class AuthFactory {
 public:
  static AuthFactory* GetInstance() {
    static AuthFactory instance;
    return &instance;
  }

  /// @brief Registers an auth instance.
  /// @note: The registration method is currently not thread-safe, and registration needs to be completed in advance
  /// during program initialization.
  bool Register(const AuthPtr &auth);

  /// @brief Gets an auth instance by |auth_name|.
  AuthPtr Get(const std::string &auth_name);

  /// @brief Get all auth plugins
  const std::unordered_map<std::string, AuthPtr>& GetAllAuthPlugins();

  /// @brief Clears the auth instances.
  /// @note Used internally by the framework.
  void Clear();

 private:
  AuthFactory() = default;

  AuthFactory(const AuthFactory&) = delete;
  AuthFactory& operator=(const AuthFactory&) = delete;

 private:
  std::unordered_map<std::string, AuthPtr> auths_;
};

}  // namespace trpc
