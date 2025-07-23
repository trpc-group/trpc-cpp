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

#include <iostream>
#include <vector>

#include "trpc/common/config/local_file_provider_conf.h"
#include "trpc/common/config/local_file_provider_conf_parser.h"
#include "trpc/util/log/logging.h"

namespace trpc {

bool GetAllLocalFileProviderConfig(std::vector<LocalFileProviderConfig>& config_list) {
  YAML::Node provider_list;
  if(!GetLocalFileProvidersNode(provider_list)) {
    return false;
  }
  for (const auto &provider : provider_list) {
    LocalFileProviderConfig config;
    // Convert node to EtcdProviderConfig
    if (!YAML::convert<LocalFileProviderConfig>::decode(provider, config)) {
      TRPC_FMT_ERROR("Convert local file config error!");
      return false;
    }
    config_list.push_back(std::move(config));
  }
  return true;
}

/// @brief Print the configuration
void LocalFileProviderConfig::Display() const {
  std::cout << "filename:" << filename << std::endl;
  std::cout << "poll_interval:" << poll_interval << std::endl;
}

}  // namespace trpc
