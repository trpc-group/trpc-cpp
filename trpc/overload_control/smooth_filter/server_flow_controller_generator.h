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

#pragma once
#include "trpc/overload_control/smooth_filter/server_flow_controller_conf.h"
#include "trpc/overload_control/smooth_filter/server_overload_controller.h"

#include <cstdint>
#include <memory>
#include <string>

namespace trpc::overload_control {

/// @brief Names of currently supported flow controllers.
constexpr char LimiterDefault[] = "default";
constexpr char LimiterSmooth[] = "smooth";

/// @brief Register the flow controller object for the service.
/// @param flow_conf Flow control conf.
void Server_RegisterFlowController(const Server_FlowControlLimiterConf& flow_conf);

/// @brief Service-level description string, format specification: name (maximum limit per second).
///        such as: default(100000), seconds(100000), smooth(100000)
/// @param limiter_desc Description string, format specification: name (maximum limit per second).
/// @param is_report Whether to report monitoring data. default value: false
/// @param window_size The size of algorithm sampling window.
/// @return If the construction fails, return nullptr. Otherwise, return a pointer to the flow controller object.
ServerOverloadControllerPtr Server_CreateFlowController(const std::string& key_name, const std::string& limiter_desc,
                                                        bool is_report = false, int32_t window_size = 0);

}  // namespace trpc::overload_control
#endif
