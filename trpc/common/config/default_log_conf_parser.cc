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
  if (!trpc::ConfigHelper::GetInstance()->GetConfig({"plugins", "log", trpc::kTrpcLogCacheStringDefault},
                                                    default_log_node)) {
    return false;
  }
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

bool GetDefaultLoggerSinkNode(std::string_view logger_name, std::string_view sink_type, std::string_view sink_name,
                              YAML::Node& sink_log_node) {
  YAML::Node logger_node, sinks_node;
  // Check if logger is configured
  if (!GetLoggerNode(logger_name, logger_node)) {
    std::cerr << "Get loggerNode err, logger_name: " << logger_name << std::endl;
    return false;
  }
  // Check if sinks are configured
  if (!trpc::ConfigHelper::GetNode(logger_node, {sink_type.data()}, sinks_node)) {
    std::cerr << logger_name << " -> " << sink_type << " not found!" << std::endl;
    return false;
  }
  // Check if sink_log is configured
  if (!trpc::ConfigHelper::GetNode(sinks_node, {sink_name.data()}, sink_log_node)) {
    std::cerr << logger_name << " -> " << sink_type << " -> " << sink_name << " not found!" << std::endl;
    return false;
  }
  return true;
}

}  // namespace YAML
