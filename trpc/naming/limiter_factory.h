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

#pragma once

#include <string>
#include <unordered_map>

#include "trpc/naming/limiter.h"

namespace trpc {

/// @brief Limiter plugin factory
class LimiterFactory {
 public:
  /// @brief Get the singleton instance of the LimiterFactory class
  /// @return LimiterFactory* A pointer to the singleton instance of the LimiterFactory class
  static LimiterFactory* GetInstance() {
    static LimiterFactory instance;
    return &instance;
  }

  LimiterFactory(const LimiterFactory&) = delete;
  LimiterFactory& operator=(const LimiterFactory&) = delete;

  /// @brief Register a Limiter plugin with the factory
  /// @param limiter The Limiter plugin to register
  /// @return true: success, false: failed
  bool Register(const LimiterPtr& limiter);

  /// @brief Get a Limiter plugin with the given name
  /// @param limiter_name The name of the Limiter plugin to get
  /// @return LimiterPtr The Limiter plugin with the given name
  LimiterPtr Get(const std::string& limiter_name);

  /// @brief Get all loadbalance plugins
  const std::unordered_map<std::string, LimiterPtr>& GetAllPlugins();

  /// @brief Clear all registered Limiter plugins from the factory
  void Clear();

 private:
  LimiterFactory() = default;

 private:
  std::unordered_map<std::string, LimiterPtr> limiters_;
};

}  // namespace trpc
