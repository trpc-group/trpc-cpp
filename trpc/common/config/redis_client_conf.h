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

#include <cstdint>
#include <string>

#include "yaml-cpp/yaml.h"

namespace trpc {

/// @brief Client config for accessing redis
/// Mainly contains authentication information
struct RedisClientConf {
  /// @brief User nmae
  std::string user_name;

  /// @brief user password
  std::string password;

  /// @brief db name(0-15)
  uint32_t db{0};

  /// @brief Whether enable auth
  bool enable{false};

  void Display() const;
};

}  // namespace trpc
