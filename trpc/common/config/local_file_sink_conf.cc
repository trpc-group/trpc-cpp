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

#include <any>
#include <iostream>

#include "trpc/common/config/default_log_conf_parser.h"
#include "trpc/common/config/local_file_sink_conf.h"
#include "trpc/common/config/local_file_sink_conf_parser.h"

#include "yaml-cpp/yaml.h"

namespace trpc {

/// @brief Print the configuration
void LocalFileSinkConfig::Display() const {
  std::cout << "format: " << format << std::endl;
  std::cout << "eol: " << format << std::endl;
  std::cout << "filename: " << format << std::endl;
  std::cout << "roll_type: " << format << std::endl;
  std::cout << "reserve_count: " << format << std::endl;
  std::cout << "roll_size by size: " << format << std::endl;
  std::cout << "rotation_hour by day: " << format << std::endl;
  std::cout << "rotation_minute by day: " << format << std::endl;
  std::cout << "remove_timeout_file_switch by day: " << format << std::endl;
  std::cout << "hour_interval by hour: " << format << std::endl;
}

}  // namespace trpc
