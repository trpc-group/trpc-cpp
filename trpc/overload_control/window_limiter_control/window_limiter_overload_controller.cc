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

#include "trpc/overload_control/window_limiter_control/window_limiter_overload_controller.h"

#include <cmath>
#include <chrono>
#include <cstdint>

#include "trpc/common/config/trpc_config.h"
#include "trpc/overload_control/common/report.h"
#include "trpc/overload_control/flow_control/flow_controller_generator.h"

namespace trpc::overload_control {

void LoadWindowLimitControlConf(std::vector<FlowControlLimiterConf>& flow_control_confs) {
  YAML::Node flow_control_nodes;
  FlowControlLimiterConf flow_control_conf;
  if (ConfigHelper::GetInstance()->GetConfig({"plugins", kOverloadCtrConfField, kWindowLimiterControlName},
                                             flow_control_nodes)) {
    for (const auto& node : flow_control_nodes) {
      auto flow_control_conf = node.as<FlowControlLimiterConf>();
      flow_control_confs.emplace_back(std::move(flow_control_conf));
    }
  }
}

bool WindowLimiterOverloadController::Init() {
  std::vector<FlowControlLimiterConf> flow_control_confs;
  LoadWindowLimitControlConf(flow_control_confs);
  for (const auto& flow_conf : flow_control_confs) {
    if (!flow_conf.service_limiter.empty()) {
      FlowControllerPtr service_controller =
          CreateFlowController(flow_conf.service_limiter, flow_conf.is_report, flow_conf.window_size);
      if (service_controller) {
        RegisterLimiter(flow_conf.service_name, service_controller);
      } else {
        TRPC_FMT_ERROR("create service window limit control fail|service_name: {}, |service_limiter: {}",
                       flow_conf.service_name, flow_conf.service_limiter);
      }
    }

    for (const auto& func_conf : flow_conf.func_limiters) {
      if (!func_conf.limiter.empty()) {
        std::string service_func_name = fmt::format("/{}/{}", flow_conf.service_name, func_conf.name);
        FlowControllerPtr func_controller =
            CreateFlowController(func_conf.limiter, flow_conf.is_report, func_conf.window_size);
        if (func_controller) {
          RegisterLimiter(service_func_name, func_controller);
        } else {
          TRPC_FMT_ERROR("create func window limit control fail|service_name:{}|func_name:{}|limiter:{}",
                         flow_conf.service_name, func_conf.name, func_conf.limiter);
        }
      }
    }
  }
  return 0;
}

bool WindowLimiterOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  auto service_controller = GetLimiter(context->GetCalleeName());
  // func flow controller
  auto func_controller = GetLimiter(context->GetFuncName());
  if (!service_controller && !func_controller) {
    return true;
  }

  // flow control strategy
  if (service_controller && service_controller->CheckLimit(context)) {
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server window limit overload control"));
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server window limit overload , service name: {}", context->GetCalleeName());
    return false;
  }
  if (func_controller && func_controller->CheckLimit(context)) {
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server window limit overload control"));
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server window limit overload , service name: {}, func name: {}",
                                context->GetCalleeName(), context->GetFuncName());
    return false;
  }
  return true;
}

void WindowLimiterOverloadController::Destroy() {
  for (auto window_limiters_iter : window_limiters_) {
    window_limiters_iter.second.reset();
  }
  window_limiters_.clear();
}

void WindowLimiterOverloadController::Stop() {
    // nothing to do,The time thread automatically stops.
};

WindowLimiterOverloadController::WindowLimiterOverloadController() {}

void WindowLimiterOverloadController::RegisterLimiter(const std::string& name, FlowControllerPtr limiter) {
  if (window_limiters_.count(name) == 0) {
    window_limiters_[name] = limiter;
  }
}

FlowControllerPtr WindowLimiterOverloadController::GetLimiter(const std::string& name) {
  FlowControllerPtr ret = nullptr;
  auto iter = window_limiters_.find(name);
  if (iter != window_limiters_.end()) {
    ret = iter->second;
  }
  return ret;
}

// The destructor does nothing, but the stop method in public needs to set the timed task to join and make it invalid
// The user must stop before calling destroy
WindowLimiterOverloadController::~WindowLimiterOverloadController() {}

}  // namespace trpc::overload_control
#endif
