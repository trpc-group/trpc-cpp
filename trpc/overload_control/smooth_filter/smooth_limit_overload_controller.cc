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
#include <chrono>
#include <cmath>
#include <cstdint>

#include "trpc/overload_control/smooth_filter/smooth_limits_overload_controller.h"
#include "trpc/overload_control/common/report.h"
#include "trpc/overload_control/flow_control/flow_controller_conf.h"
#include "trpc/overload_control/flow_control/flow_controller_generator.h"

#include "trpc/util/log/logging.h"

namespace trpc::overload_control {

bool SmoothLimitOverloadController::Init() {
std::vector<FlowControlLimiterConf> flow_control_confs;
  LoadFlowControlLimiterConf(flow_control_confs);
  for (const auto& flow_conf : flow_control_confs) {
    if (!flow_conf.service_limiter.empty()) {
    FlowControllerPtr service_controller =
        CreateFlowController(flow_conf.service_limiter, flow_conf.is_report, flow_conf.window_size);
    if (service_controller) {
      RegisterLimiter(flow_conf.service_name, service_controller);
    } else {
      TRPC_FMT_ERROR("create service flow control fail|service_name: {}, |service_limiter: {}", flow_conf.service_name,
                     flow_conf.service_limiter);
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
        TRPC_FMT_ERROR("create func flow control fail|service_name:{}|func_name:{}|limiter:{}", flow_conf.service_name,
                       func_conf.name, func_conf.limiter);
      }
    }
  }
  }
  return 0;
}

bool SmoothLimitOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  auto service_controller = GetLimiter(context->GetCalleeName());
  // func flow controller
  auto func_controller = GetLimiter(context->GetFuncName());
  if (!service_controller && !func_controller) {
    return true;
  }

  // flow control strategy
  if (service_controller && service_controller->CheckLimit(context)) {
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server haoyuflow overload control"));
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server haoyuflow overload , service name: {}", context->GetCalleeName());
    return false;
  }
  if (func_controller && func_controller->CheckLimit(context)) {
    context->SetStatus(
        Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server haoyuflow overload control"));
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server haoyuflow overload , service name: {}, func name: {}",
                                context->GetCalleeName(), context->GetFuncName());
    return false;
  }
  return true;
}

void SmoothLimitOverloadController::Destroy(){
    for(auto smooth_limits_iter : smooth_limits_){
        smooth_limits_iter.second.reset();
    }
}

void SmoothLimitOverloadController::Stop(){
    // nothing to do,The time thread automatically stops.
};

SmoothLimitOverloadController::SmoothLimitOverloadController()
{}

void SmoothLimitOverloadController::RegisterLimiter(const std::string& name,FlowControllerPtr limiter){
    if(smooth_limits_.count(name) == 0)
        smooth_limits_[name] = limiter;
}

FlowControllerPtr SmoothLimitOverloadController::GetLimiter(const std::string& name) {
  FlowControllerPtr ret = nullptr;
  auto iter = smooth_limits_.find(name);
  if (iter != smooth_limits_.end()) {
    ret = iter->second;
  }
  return ret;
}

//The destructor does nothing, but the stop method in public needs to set the timed task to join and make it invalid
//The user must stop before calling destroy
SmoothLimitOverloadController::~SmoothLimitOverloadController() {}

}  // namespace trpc::overload_control
#endif

