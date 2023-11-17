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
#include <iostream>

#include "trpc/common/config/default_log_conf_parser.h"
#include "trpc/common/config/stdout_sink_conf.h"
#include "trpc/common/config/stdout_sink_conf_parser.h"

#include "yaml-cpp/yaml.h"

namespace trpc {

/// @brief Print the configuration
void StdoutSinkConfig::Display() const {
  std::cout << "format: " << format << std::endl;
  std::cout << "eol: " << eol << std::endl;
  std::cout << "stderr_level: " << stderr_level << std::endl;
}

}  // namespace trpc
