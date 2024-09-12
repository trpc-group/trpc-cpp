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
#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/seconds_limiter/seconds_overload_controller.h"

#include "trpc/overload_control/flow_control/flow_controller_generator.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/util/log/logging.h"

namespace trpc::overload_control {

bool SecondsOverloadController::Init(const std::vector<FlowControlLimiterConf>& flow_confs) {
  for (auto flow_conf : flow_confs) {
    if (!flow_conf.service_limiter.empty()) {
      SecondsLimiterPtr seconds_limiter = std::dynamic_pointer_cast<SecondsLimiter>(
          CreateFlowController(flow_conf.service_limiter, flow_conf.is_report, flow_conf.window_size));
      if (seconds_limiter) {
        Register(flow_conf.service_name, seconds_limiter);
      } else {
        TRPC_FMT_ERROR("create service seconds limiter fail||service_name: {}, |service_limiter: {}",
                       flow_conf.service_name, flow_conf.service_limiter);
      }
    }

    for (const auto& func_conf : flow_conf.func_limiters) {
      if (!func_conf.limiter.empty()) {
        std::string service_func_name = fmt::format("/{}/{}", flow_conf.service_name, func_conf.name);
        SecondsLimiterPtr func_limiter = std::dynamic_pointer_cast<SecondsLimiter>(
            CreateFlowController(func_conf.limiter, flow_conf.is_report, func_conf.window_size));
        if (func_limiter) {
          Register(service_func_name, func_limiter);
        } else {
          TRPC_FMT_ERROR("create func seconds limiter fail|service_name:{}|func_name:{}|limiter:{}",
                         flow_conf.service_name, func_conf.name, func_conf.limiter);
        }
      }
    }
  }
  return true;
}

bool SecondsOverloadController::BeforeSchedule(const ServerContextPtr& context) {
  auto service_limiter = GetLimiter(context->GetCalleeName());
  auto func_limiter = GetLimiter(context->GetFuncName());
  if (!service_limiter && !func_limiter) {
    return false;
  }

  // flow control strategy
  if (service_limiter && service_limiter->CheckLimit(context)) {
    context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server flow overload control"));
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server flow overload , service name: {}", context->GetCalleeName());
    return true;
  }
  if (func_limiter && func_limiter->CheckLimit(context)) {
    context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server flow overload control"));
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server flow overload , service name: {}, func name: {}",
                                context->GetCalleeName(), context->GetFuncName());
    return true;
  }
  return false;
}

bool SecondsOverloadController::AfterSchedule(const ServerContextPtr& context) { return false; }

void SecondsOverloadController::Stop() {}

void SecondsOverloadController::Destroy() {}

void SecondsOverloadController::Register(const std::string& name, const SecondsLimiterPtr& limiter) {
  limiter_map_.emplace(name, limiter);
}

SecondsLimiterPtr SecondsOverloadController::GetLimiter(const std::string& name) {
  SecondsLimiterPtr ret = nullptr;
  auto iter = limiter_map_.find(name);
  if (iter != limiter_map_.end()) {
    ret = iter->second;
  }
  return ret;
}

}  // namespace trpc::overload_control

#endif