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

#include "trpc/common/plugin.h"

namespace trpc {

/// @brief The tRPC Config plugin abstract interface definition class, targeting plugin developers.
/// @note This class is reserved for 1.0 compatibility, and new data sources
///       should inherit provider_base.h directly.
class Config : public Plugin {
 public:
  /// @brief Default constructor and destructor.
  Config() = default;
  ~Config() override = default;

  /// @brief Pulls file configuration.
  /// @param config_name - The Group name.
  /// @param config - The content of the obtained configuration.
  /// @param params - File name.
  /// @return - true: Loading successful.
  ///           false: Loading failed.
  virtual bool PullFileConfig(const std::string& config_name, std::string* config, const std::any& params) = 0;

  /// @brief Pulls a single key-value configuration.
  /// @param config_name - The filename of the KV configuration to be loaded, means group.
  /// @param key - Input key.
  /// @param config - Obtains the value corresponding to the key.
  /// @return - true: Loading successful.
  ///           false: Loading failed.
  virtual bool PullKVConfig(const std::string& config_name, const std::string& key, std::string* config,
                            const std::any& params) = 0;

  /// @brief Pulls multiple key-value configurations.
  /// @param config_name - The filename of the KV configuration to be loaded, means group.
  /// @param config - Obtains key-value pairs.
  /// @return - true: Loading successful.
  ///           false: Loading failed.
  virtual bool PullMultiKVConfig(const std::string& config_name, std::map<std::string, std::string>* config,
                                 const std::any& params) = 0;

  /// @brief Sets an asynchronous callback function to notify configuration updates.
  virtual void SetAsyncCallBack(const std::any& param) {}
};

using ConfigPtr = RefPtr<Config>;

}  // namespace trpc
