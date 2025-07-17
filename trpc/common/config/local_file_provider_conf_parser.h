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

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/config_helper.h"
#include "trpc/common/config/local_file_provider_conf.h"

namespace YAML {

inline bool GetLocalFileProvidersNode(YAML::Node& providers) {
  if (!trpc::ConfigHelper::GetInstance()->GetNode(
        {"plugins", "config", "local_file", "providers"}, providers)) {
    return false;
  }
  return true;
}

template <>
struct convert<trpc::LocalFileProviderConfig> {
  static YAML::Node encode(const trpc::LocalFileProviderConfig& file_conf) {
    YAML::Node node;
    node["filename"] = file_conf.filename;
    node["poll_interval"] = file_conf.poll_interval;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::LocalFileProviderConfig& file_conf) {  // NOLINT
    if (node["name"]) {
      file_conf.name = node["name"].as<std::string>();
    }
    if (node["filename"]) {
      file_conf.filename = node["filename"].as<std::string>();
    }
    if (node["poll_interval"]) {
      file_conf.poll_interval = node["poll_interval"].as<unsigned int>();
    }
    return true;
  }
};

}  // namespace YAML
