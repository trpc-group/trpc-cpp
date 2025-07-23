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

#include "trpc/config/provider.h"
#include "trpc/util/log/logging.h"

namespace trpc::config {

/// @brief The DataProviderFactory class is a factory class for data providers.
class ProviderFactory {
 public:
  /// @brief Gets the global singleton instance of the factory.
  /// @return A pointer to the DataProviderFactory instance.
  static ProviderFactory* GetInstance() {
    static ProviderFactory instance;
    return &instance;
  }

  /// @brief Disallow copy and assign constructors.
  ProviderFactory(const ProviderFactory&) = delete;
  ProviderFactory& operator=(const ProviderFactory&) = delete;

  /// @brief Registers a data provider with the factory.
  /// @param provider The data provider to register.
  /// @return true: success, false: failed.
  bool Register(config::ProviderPtr provider) {
    TRPC_ASSERT(provider != nullptr);
    config_providers_[provider->Name()] = std::move(provider);
    return true;
  }

  /// @brief Gets a data provider by name.
  /// @param name The name of the data provider to get.
  /// @return The data provider with the given name, or nullptr if not found.
  config::ProviderPtr Get(const std::string& name) const {
    auto it = config_providers_.find(name);
    if (it != config_providers_.end()) {
      return it->second;
    }
    return nullptr;
  }

  /// @brief Get all config provider plugins
  const std::unordered_map<std::string, config::ProviderPtr>& GetAllPlugins() { return config_providers_; }

  /// @brief Clear all config provider plugins
  void Clear() { config_providers_.clear(); }

 private:
  // Constructs a DataProviderFactory object.
  ProviderFactory() = default;

 private:
  // key is the name of the provider, value is the provider object
  std::unordered_map<std::string, config::ProviderPtr> config_providers_;
};

}  // namespace trpc::config
