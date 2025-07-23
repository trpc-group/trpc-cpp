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

#include "trpc/common/config/redis_client_conf.h"

namespace YAML {

template <>
struct convert<trpc::RedisClientConf> {
  static YAML::Node encode(const trpc::RedisClientConf& redis_conf) {
    YAML::Node node;
    node["password"] = redis_conf.password;
    node["user_name"] = redis_conf.user_name;
    node["db"] = redis_conf.db;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::RedisClientConf& redis_conf) {  // NOLINT
    if (node["password"]) {
      redis_conf.password = node["password"].as<std::string>();
    }
    if (node["user_name"]) {
      redis_conf.user_name = node["user_name"].as<std::string>();
    }
    if (node["db"]) {
      redis_conf.db = node["db"].as<uint32_t>();
    }
    return true;
  }
};

}  // namespace YAML
