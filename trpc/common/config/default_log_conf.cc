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

#include <any>

#include "trpc/common/config/default_log_conf.h"
#include "trpc/common/config/default_log_conf_parser.h"

namespace trpc {

void DefaultLogConfig::Display() const {
  for (const auto& instance : this->instances) {
    instance.Display();
  }
}

bool GetDefaultLogConfig(trpc::DefaultLogConfig& config) {
  YAML::Node default_log_node;
  // yaml gets node by parsing DefaultLog
  if (!GetDefaultLogNode(default_log_node)) {
    std::cerr << "Get DefaultLogNode err! Please check out the log plugin." << std::endl;
    return false;
  }
  // node converts to DefaultLogConfig
  YAML::convert<DefaultLogConfig> c;
  c.decode(default_log_node, config);
  return true;
}

}  // namespace trpc
