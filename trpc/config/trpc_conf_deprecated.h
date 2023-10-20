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

#include <any>
#include <map>
#include <string>

#include "trpc/config/provider_factory.h"
#include "trpc/config/trpc_conf.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief Old interface, not recommended for further use
class TrpcConf {
 public:
  /// @brief Loads the text configuration of a file type
  /// @param plugin_name The name of the configuration plugin
  /// @param config_name The Group name
  /// @param result The loaded text configuration content
  /// @param params The configuration request information
  /// @return bool true: if the configuration is loaded successfully
  ///              false: if the configuration fails to load
  static bool LoadConfig(const std::string& plugin_name, const std::string& config_name, std::string& result,
                         const std::any& params = nullptr) {
    auto provider = ProviderFactory::GetInstance()->Get(plugin_name);
    if (!provider) {
      TRPC_FMT_ERROR("Plugin not registered, plugin_name:{}", plugin_name);
      return false;
    }

    trpc::config::DefaultConfigPtr cfg = trpc::config::Load(config_name, config::WithProvider(plugin_name));
    if (!cfg) {
      TRPC_FMT_ERROR("get config fail, plugin_name:{}", plugin_name);
    }

    result = cfg->GetRawData();
    TRPC_FMT_DEBUG("raw_config:{} ", result);

    return true;
  }

  /// @brief Loads all key-value pairs of a kv type configuration
  /// @param plugin_name The name of the configuration plugin
  /// @param config_name The Group name
  /// @param result The loaded key-value pairs content
  /// @param params The configuration request information
  /// @return bool true: if the configuration is loaded successfully
  ///              false: if the configuration fails to load
  static bool LoadKvConfig(const std::string& plugin_name, const std::string& config_name,
                           std::map<std::string, std::string>& result, const std::any& params = nullptr) {
    ProviderPtr provider = ProviderFactory::GetInstance()->Get(plugin_name);
    if (!provider) {
      TRPC_FMT_ERROR("Plugin not registered, plugin_name:{}", plugin_name);
      return false;
    }

    config::DefaultConfigPtr cfg = config::Load(config_name, WithCodec("yaml"), WithProvider(plugin_name));
    if (!config) {
      TRPC_FMT_ERROR("get config fail, plugin_name:{}", plugin_name);
    }
    result = cfg->GetDecodeData();
    TRPC_FMT_DEBUG("raw_config:{} ", result);

    return true;
  }

  /// @brief Loads a single key-value pair of a kv type configuration
  /// @param plugin_name The name of the configuration plugin
  /// @param config_name The Group name
  /// @param key The name of the key to be loaded
  /// @param result The loaded value of the specified key
  /// @param params The configuration request information
  /// @return bool true: if the configuration is loaded successfully
  ///              false: if the configuration fails to load
  static bool LoadSingleKvConfig(const std::string& plugin_name, const std::string& config_name, const std::string& key,
                                 std::string& result, const std::any& params = nullptr) {
    config::ProviderPtr provider = ProviderFactory::GetInstance()->Get(plugin_name);
    if (!provider) {
      TRPC_FMT_ERROR("Plugin not registered, plugin_name:{}", plugin_name);
      return false;
    }

    config::DefaultConfigPtr cfg =
        config::Load(config_name, config::WithCodec("yaml"), config::WithProvider(plugin_name));
    if (!config) {
      TRPC_FMT_ERROR("get config fail, plugin_name:{}", plugin_name);
    }

    result = cfg->GetString(key, "");
    TRPC_FMT_DEBUG("raw_config:{} ", result);

    return true;
  }
};

}  // namespace trpc
