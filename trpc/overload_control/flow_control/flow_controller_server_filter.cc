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

#include "trpc/overload_control/flow_control/flow_controller_server_filter.h"

#include "trpc/codec/codec_helper.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/log/trpc_log.h"
#include "trpc/overload_control/flow_control/flow_controller_conf.h"
#include "trpc/overload_control/flow_control/flow_controller_factory.h"
#include "trpc/util/likely.h"

namespace trpc::overload_control {

int FlowControlServerFilter::Init() {
  std::vector<FlowControlLimiterConf> flow_control_confs;
  LoadFlowControlLimiterConf(flow_control_confs);
  for (const auto& conf : flow_control_confs) {
    RegisterFlowController(conf);
  }
  return 0;
}

std::vector<FilterPoint> FlowControlServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_POST_RECV_MSG,
      // This tracking point is not used, but tracking points must be paired, so it is added here.
      FilterPoint::SERVER_PRE_SEND_MSG,
  };
}

void FlowControlServerFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
  switch (point) {
    case FilterPoint::SERVER_POST_RECV_MSG: {
      OnRequest(status, context);
      break;
    }
    default: {
      break;
    }
  }
}

void FlowControlServerFilter::OnRequest(FilterStatus& status, const ServerContextPtr& context) {
  if (TRPC_UNLIKELY(!context->GetStatus().OK())) {
    // If it is already a dirty request, it will not be processed further to ensure that the first error code is
    // not overwritten.
    return;
  }
  // ServiceImpl flow controller
  auto service_controller = FlowControllerFactory::GetInstance()->GetFlowController(context->GetCalleeName());
  // func flow controller
  auto func_controller = FlowControllerFactory::GetInstance()->GetFlowController(context->GetFuncName());
  if (!service_controller && !func_controller) {
    return;
  }

  // flow control strategy
  if (service_controller && service_controller->CheckLimit(context)) {
    context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server flow overload control"));
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server flow overload , service name: {}", context->GetCalleeName());
    status = FilterStatus::REJECT;
    return;
  }
  if (func_controller && func_controller->CheckLimit(context)) {
    context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "rejected by server flow overload control"));
    status = FilterStatus::REJECT;
    TRPC_FMT_ERROR_EVERY_SECOND("rejected by server flow overload , service name: {}, func name: {}",
                                context->GetCalleeName(), context->GetFuncName());
    return;
  }
}

}  // namespace trpc::overload_control

#endif
