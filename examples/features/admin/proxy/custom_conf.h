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

namespace examples::admin {

struct CustomConfig {
  int value = 0;
};

}  // namespace examples::admin

namespace YAML {

template <>
struct convert<examples::admin::CustomConfig> {
  static YAML::Node encode(const examples::admin::CustomConfig& conf) {
    YAML::Node node;
    node["value"] = conf.value;

    return node;
  }

  static bool decode(const YAML::Node& node, examples::admin::CustomConfig& conf) {
    if (node["value"]) {
      conf.value = node["value"].as<int>();
    }

    return true;
  }
};

}  // namespace YAML
