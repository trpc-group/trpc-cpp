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

#include "trpc/naming/registry.h"

namespace trpc {

/// @brief Service registry plugin factory
class RegistryFactory {
 public:
  /// @brief Get the singleton instance of the RegistryFactory class
  /// @return RegistryFactory* A pointer to the singleton instance of the RegistryFactory class
  static RegistryFactory* GetInstance() {
    static RegistryFactory instance;
    return &instance;
  }

  RegistryFactory(const RegistryFactory&) = delete;
  RegistryFactory& operator=(const RegistryFactory&) = delete;

  /// @brief Register a Service Registry plugin with the factory
  /// @param registry The Service Registry plugin to register
  /// @return true: success, false: failed
  bool Register(const RegistryPtr& registry);

  /// @brief Get a Service Registry plugin with the given name
  /// @param registry_name The name of the Service Registry plugin to get
  /// @return RegistryPtr& The Service Registry plugin with the given name
  RegistryPtr Get(const std::string& registry_name);

  /// @brief Get all registry plugins
  const std::unordered_map<std::string, RegistryPtr>& GetAllPlugins();

  /// @brief Clear all registered Service Registry plugins from the factory
  void Clear();

 private:
  RegistryFactory() = default;

 private:
  std::unordered_map<std::string, RegistryPtr> registrys_;
};

}  // namespace trpc
