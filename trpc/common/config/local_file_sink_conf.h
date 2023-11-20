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
#include <string_view>

#include "trpc/common/config/default_log_conf.h"

namespace trpc {

/// @brief The local file outputs the logging configuration.
struct LocalFileSinkConfig {
  /// @brief Log format
  std::string format;
  /// @brief Whether to newline
  bool eol{true};
  /// @brief File name
  std::string filename{"trpc.log"};
  /// @brief by_size- Split by size, default; by_day- Split by day; by_hour- Split by hour
  std::string roll_type{"by_size"};
  /// @brief Keep log file count
  unsigned int reserve_count{10};
  /// @brief Scroll log size
  unsigned int roll_size{1024 * 1024 * 100};
  /// @brief Represents the time of day, as specified by rotation_hour:rotation_minute
  unsigned int rotation_hour{0};
  /// @brief Cut by minutes
  unsigned int rotation_minute{0};
  /// @brief In split by day mode, remove the identity of obsolete files, not by default
  bool remove_timeout_file_switch{false};

  /// @brief Represents an hourly interval in hours, default interval is one hour
  unsigned int hour_interval{1};

  /// @brief Print the configuration
  void Display() const;
};

}  // namespace trpc
