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

#include <map>
#include <string>

#include "trpc/config/default/loader.h"

namespace trpc::config {

/// @brief Initialize the inner "codec", "provider" plugin
bool Init();

/// @brief Stop the inner "codec", "provider" plugin
void Stop();

/// @brief Destroy the inner "codec", "provider" plugin
void Destroy();

/// @brief Loads the configuration data from the specified path.
/// @tparam Args The types of the options to use.
/// @param path The path of the configuration file to load.
/// @param opts The load options to use.
/// @return A shared pointer to the loaded ConfigBase object.

/// Example usage:
/// @code{.cpp}
/// auto db_cfg = trpc::config::Load("test.yaml", trpc::config::WithCodec("yaml"),
/// trpc::config::WithProvider("rainbow1")); std::string username = db_cfg.GetString("username", "trpc-db1");
/// std::string password = db_cfg.GetString("password", "123456");
/// @endcode
template <typename... Args>
DefaultConfigPtr Load(const std::string& path, Args&&... opts) {
  static_assert((std::is_same_v<Args, LoadOptions> && ...), "Args must be LoadOption objects");
  // Hand it to the tPRC-CPP default loader
  std::vector<LoadOptions> options = {std::forward<LoadOptions>(opts)...};
  return detail::Load(path, std::move(options));
}

}  // namespace trpc::config
