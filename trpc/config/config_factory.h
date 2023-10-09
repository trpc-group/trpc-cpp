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

#include "trpc/config/config.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief The ConfigFactory class is a factory class for tRPC configuration plugins.
class ConfigFactory {
 public:
  /// @brief Gets the global singleton instance of the factory.
  /// @return A pointer to the ConfigFactory instance.
  static ConfigFactory* GetInstance() {
    static ConfigFactory instance;
    return &instance;
  }

  /// @brief Disallow copy and assign constructors.
  ConfigFactory(const ConfigFactory&) = delete;
  ConfigFactory& operator=(const ConfigFactory&) = delete;

  /// @brief Registers a configuration plugin with the factory.
  /// @note The modification of config_plugins_ is done by the framework during initialization
  ///       by calling the initialization function of the configuration center to complete registration.
  ///       The operation of getting config_plugins_ can ensure that it is after initialization,
  ///       so the operation of config_plugins_ does not need to be locked.
  ///       Of course, it is not thread-safe.
  /// @param config The configuration plugin to register.
  /// @return true: success, false: failed.
  bool Register(const ConfigPtr& config) {
    TRPC_ASSERT(config != nullptr);
    config_plugins_[config->Name()] = config;

    return true;
  }

  /// Gets a configuration plugin by name.
  /// @param name The name of the configuration plugin to get.
  /// @return The configuration plugin with the given name, or nullptr if not found.
  ConfigPtr Get(const std::string& name) const {
    auto it = config_plugins_.find(name);
    if (it != config_plugins_.end()) {
      return it->second;
    }

    return nullptr;
  }

  /// @brief Get all config plugins
  const std::unordered_map<std::string, ConfigPtr>& GetAllPlugins() { return config_plugins_; }

  /// @brief Clears the members of the plugin factory map.
  void Clear() { config_plugins_.clear(); }

 private:
  // Constructs a ConfigFactory object.
  ConfigFactory() = default;

 private:
  // key is the name of the plugin, value is the plugin object
  std::unordered_map<std::string, ConfigPtr> config_plugins_;
};

}  // namespace trpc
