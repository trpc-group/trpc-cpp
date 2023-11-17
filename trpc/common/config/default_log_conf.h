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

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace trpc {

/// @brief default log configuration
struct DefaultLogConfig {
  /// @brief logger instance configuration
  struct LoggerInstance {
    /// @brief logger instance name
    std::string name;
    /// @brief logger level
    unsigned int min_level{2};
    /// @brief logger output format
    std::string format{"[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v"};
    /// @brief logger output mode
    unsigned int mode{2};  // 1: sync 2: async 3: overrun_oldest

    /// @brief Print out the logger configuration.
    void Display() const {
      std::cout << "name: " << name << std::endl;
      std::cout << "min_level: " << min_level << std::endl;
      std::cout << "format: " << format << std::endl;
      std::cout << "mode: " << mode << " ===> 1: sync 2: async 3: overrun_oldest" << std::endl;
    }
  };

  /// @brief list of logger instance
  std::vector<LoggerInstance> instances;

  /// @brief Print out all the logger configurations
  void Display() const;
};

/// @brief Get node config for all loggers under default plugin
bool GetDefaultLogConfig(trpc::DefaultLogConfig& config);

}  // namespace trpc
