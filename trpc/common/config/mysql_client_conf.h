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

#include "trpc/common/config/mysql_connect_pool_conf.h"
#include "yaml-cpp/yaml.h"
namespace trpc {

/// @brief Client config for accessing mysql
/// Mainly contains authentication information
struct MysqlClientConf {
  /// @brief User nmae
  std::string user_name;

  /// @brief user password
  std::string password;

  /// @brief db name
  std::string dbname;

  /// @brief ip
  std::string ip;

  /// @brief port
  std::string port;

  /// @brief Whether enable auth
  bool enable{true};

  /// @brief mysql connect pool config
  MysqlConnectPoolConf connectpool;

  void Display() const;
};

}  // namespace trpc
