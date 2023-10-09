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

#include "trpc/common/config/default_log_conf_parser.h"

namespace YAML {

bool GetDefaultLogNode(YAML::Node& default_log_node) {
  if (!trpc::ConfigHelper::GetInstance()->GetConfig({"plugins", "log", "default"}, default_log_node)) return false;
  return true;
}

bool GetLoggerNode(std::string_view logger_name, YAML::Node& logger_node) {
  YAML::Node default_log_node;
  if (!GetDefaultLogNode(default_log_node)) return false;
  std::vector<YAML::Node> children;
  auto ret =
      trpc::ConfigHelper::FilterNode(default_log_node, children, [&logger_name](size_t idx, const YAML::Node& child) {
        return child["name"] && child["name"].as<std::string>() == logger_name;
      });
  if (!ret && children.size() != 1) return false;
  logger_node.reset(children.front());

  return true;
}

}  // namespace YAML
