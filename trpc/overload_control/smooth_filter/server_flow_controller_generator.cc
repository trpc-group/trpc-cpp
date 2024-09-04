//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/smooth_filter/server_flow_controller_generator.h"

#include <vector>

#include "fmt/format.h"

#include "trpc/common/logging/trpc_logging.h"
#include "trpc/overload_control/smooth_filter/server_overload_controller_factory.h"
#include "trpc/overload_control/smooth_filter/server_smooth_limit.h"
#include "trpc/util/string_util.h"

namespace trpc::overload_control {

void Server_RegisterFlowController(const Server_FlowControlLimiterConf& flow_conf) {
  if (!flow_conf.service_limiter.empty()) {
    ServerOverloadControllerPtr service_controller = Server_CreateFlowController(
        flow_conf.service_name, flow_conf.service_limiter, flow_conf.is_report, flow_conf.window_size);
    if (service_controller) {
      ServerOverloadControllerFactory::GetInstance()->Register(service_controller);
    } else {
      TRPC_FMT_ERROR("create service flow control fail|service_name: {}, |service_limiter: {}", flow_conf.service_name,
                     flow_conf.service_limiter);
    }
  }

  for (const auto& func_conf : flow_conf.func_limiters) {
    if (!func_conf.limiter.empty()) {
      std::string service_func_name = fmt::format("/{}/{}", flow_conf.service_name, func_conf.name);
      ServerOverloadControllerPtr func_controller =
          Server_CreateFlowController(service_func_name, func_conf.limiter, flow_conf.is_report, func_conf.window_size);
      if (func_controller) {
        ServerOverloadControllerFactory::GetInstance()->Register(func_controller);
      } else {
        TRPC_FMT_ERROR("create func flow control fail|service_name:{}|func_name:{}|limiter:{}", flow_conf.service_name,
                       func_conf.name, func_conf.limiter);
      }
    }
  }
}

ServerOverloadControllerPtr Server_CreateFlowController(const std::string& key_name, const std::string& limiter_desc,
                                                        bool is_report, int32_t window_size) {
  // Format: name (maximum limit per second).
  std::string str = trpc::util::Trim(limiter_desc);

  // The trailing character must be ')'.
  if (str.length() < 1 || str.at(str.length() - 1) != ')') {
    return nullptr;
  }

  // Covert to 'default(100000'
  str = str.substr(0, str.length() - 1);

  // Split the fields using '('.
  std::vector<std::string> items = trpc::util::SplitString(limiter_desc, '(');
  if (items.size() != 2) {
    return nullptr;
  }

  // The name must not be empty, and the limit number must not be empty and valid.
  std::string limiter_name = trpc::util::Trim(items[0]);
  std::string limiter_rps = trpc::util::Trim(items[1]);
  if (limiter_name.empty() || limiter_rps.empty()) {
    return nullptr;
  }

  int64_t max_rps = atol(limiter_rps.c_str());
  if (max_rps < 1) {
    return nullptr;
  }
  // Check if the name is within the supported flow controller implementations
  if (limiter_name == LimiterDefault) {
    return ServerOverloadControllerPtr(new SmoothLimit(key_name, max_rps, is_report, window_size));
  } else if (limiter_name == LimiterSmooth) {
    return ServerOverloadControllerPtr(new SmoothLimit(key_name, max_rps, is_report, window_size));
  }

  return nullptr;
}

}  // namespace trpc::overload_control

#endif
