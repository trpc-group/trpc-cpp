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

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/load_balance_naming_conf.h"

namespace YAML {

template <>
struct convert<trpc::naming::LoadBalanceSelectorConfig> {
  static YAML::Node encode(const trpc::naming::LoadBalanceSelectorConfig& config) {}

  static bool decode(const YAML::Node& node, trpc::naming::LoadBalanceSelectorConfig& config) {}
};

}  // namespace YAML
