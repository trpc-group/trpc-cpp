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
#include <vector>

#include "trpc/common/plugin.h"

namespace trpc::config {

using ProviderCallback = void (*)(const std::string& key, const std::string& value);

/// @brief The provider class is an abstract interface definition class for data providers.
class Provider : public Plugin {
 public:
  /// @brief  Plugin Types
  /// @return Return plugin type
  PluginType Type() const override { return PluginType::kConfigProvider; }

  /// @brief Returns the data provider's name.
  /// @return The name of the data provider.
  virtual std::string Name() const = 0;

  /// @brief Reads the specific path file and returns its content as bytes.
  /// @param path The path of the file to read.
  /// @return The content of the file as bytes.
  virtual std::string Read(const std::string& path) = 0;

  // Watch watches config changing. The change will
  // be handled by callback function.
  virtual void Watch(ProviderCallback callback) = 0;
};

using ProviderPtr = RefPtr<Provider>;

}  // namespace trpc::config
