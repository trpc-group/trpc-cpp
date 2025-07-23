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
#include <string_view>

namespace trpc {

/// @brief The stdout logging configuration.
struct StdoutSinkConfig {
  /// @brief Log format
  std::string format;
  /// @brief Whether to newline
  bool eol{true};
  /// @brief Minimum log level to stderr
  unsigned int stderr_level{4};

  /// @brief Print the configuration
  void Display() const;
};

}  // namespace trpc
