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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/flow_control/flow_controller_factory.h"

namespace trpc::overload_control {

void FlowControllerFactory::Register(const std::string& name, const FlowControllerPtr& controller) {
  controller_map_.emplace(name, controller);
}

FlowControllerPtr FlowControllerFactory::GetFlowController(const std::string& name) {
  FlowControllerPtr ret = nullptr;
  auto iter = controller_map_.find(name);
  if (iter != controller_map_.end()) {
    ret = iter->second;
  }
  return ret;
}

void FlowControllerFactory::Clear() { controller_map_.clear(); }

size_t FlowControllerFactory::Size() const { return controller_map_.size(); }

}  // namespace trpc::overload_control

#endif
