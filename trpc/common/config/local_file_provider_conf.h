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

namespace trpc {

/// @brief The config for local file config provider
struct LocalFileProviderConfig {
  /// @brief Provider name
  std::string name{"default"};
  /// @brief File name
  std::string filename{"trpc_cpp.yaml"};
  /// @brief Interval of polling file content (in seconds)
  unsigned int poll_interval{1};

  /// @brief Print the configuration
  void Display() const;
};

///@brief Retrieves the configuration for all local file providers.
///@param config_list A reference to a vector where the configuration of each
///@return A boolean value indicating the success of the operation.
bool GetAllLocalFileProviderConfig(std::vector<LocalFileProviderConfig>& config_list);

}  // namespace trpc
